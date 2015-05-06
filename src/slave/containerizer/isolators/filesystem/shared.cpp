/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <set>

#include "slave/containerizer/isolators/filesystem/shared.hpp"

using namespace process;

using std::list;
using std::set;
using std::string;

namespace mesos {
namespace internal {
namespace slave {

using mesos::slave::ExecutorRunState;
using mesos::slave::Isolator;
using mesos::slave::IsolatorProcess;
using mesos::slave::Limitation;

SharedFilesystemIsolatorProcess::SharedFilesystemIsolatorProcess(
    const Flags& _flags)
  : flags(_flags) {}


SharedFilesystemIsolatorProcess::~SharedFilesystemIsolatorProcess() {}


Try<Isolator*> SharedFilesystemIsolatorProcess::create(const Flags& flags)
{
  Result<string> user = os::user();
  if (!user.isSome()) {
    return Error("Failed to determine user: " +
                 (user.isError() ? user.error() : "username not found"));
  }

  if (user.get() != "root") {
    return Error("SharedFilesystemIsolator requires root privileges");
  }

  process::Owned<IsolatorProcess> process(
      new SharedFilesystemIsolatorProcess(flags));

  return new Isolator(process);
}


Future<Nothing> SharedFilesystemIsolatorProcess::recover(
    const list<ExecutorRunState>& states,
    const hashset<ContainerID>& orphans)
{
  // There is nothing to recover because we do not keep any state and
  // do not monitor filesystem usage or perform any action on cleanup.
  return Nothing();
}


Future<Option<CommandInfo>> SharedFilesystemIsolatorProcess::prepare(
    const ContainerID& containerId,
    const ExecutorInfo& executorInfo,
    const string& directory,
    const Option<string>& user)
{
  if (executorInfo.has_container() &&
      executorInfo.container().type() != ContainerInfo::MESOS) {
    return Failure("Can only prepare filesystem for a MESOS container");
  }

  LOG(INFO) << "Preparing shared filesystem for container: "
            << stringify(containerId);

  if (!executorInfo.has_container()) {
    // We don't consider this an error, there's just nothing to do so
    // we return None.

    return None();
  }

  // We don't support mounting to a container path which is a parent
  // to another container path as this can mask entries. We'll keep
  // track of all container paths so we can check this.
  set<string> containerPaths;
  containerPaths.insert(directory);

  list<string> commands;

  foreach (const Volume& volume, executorInfo.container().volumes()) {
    // Because the filesystem is shared we require the container path
    // already exist, otherwise containers can create arbitrary paths
    // outside their sandbox.
    if (!os::exists(volume.container_path())) {
      return Failure("Volume with container path '" +
                     volume.container_path() +
                     "' must exist on host for shared filesystem isolator");
    }

    // Host path must be provided.
    if (!volume.has_host_path()) {
      return Failure("Volume with container path '" +
                     volume.container_path() +
                     "' must specify host path for shared filesystem isolator");
    }

    // Check we won't mask another volume.
    // NOTE: Assuming here that the container path is absolute, see
    // Volume protobuf.
    // TODO(idownes): This test is unnecessarily strict and could be
    // relaxed if mounts could be re-ordered.
    foreach (const string& containerPath, containerPaths) {
      if (strings::startsWith(volume.container_path(), containerPath)) {
        return Failure("Cannot mount volume to '" +
                        volume.container_path() +
                        "' because it is under volume '" +
                        containerPath +
                        "'");
      }

      if (strings::startsWith(containerPath, volume.container_path())) {
        return Failure("Cannot mount volume to '" +
                        containerPath +
                        "' because it is under volume '" +
                        volume.container_path() +
                        "'");
      }
    }
    containerPaths.insert(volume.container_path());

    // A relative host path will be created in the container's work
    // directory, otherwise check it already exists.
    string hostPath;
    if (!strings::startsWith(volume.host_path(), "/")) {
      hostPath = path::join(directory, volume.host_path());

      // Do not support any relative components in the resulting path.
      // There should not be any links in the work directory to
      // resolve.
      if (strings::contains(hostPath, "/./") ||
          strings::contains(hostPath, "/../")) {
        return Failure("Relative host path '" +
                       hostPath +
                       "' cannot contain relative components");
      }

      Try<Nothing> mkdir = os::mkdir(hostPath, true);
      if (mkdir.isError()) {
        return Failure("Failed to create host_path '" +
                        hostPath +
                        "' for mount to '" +
                        volume.container_path() +
                        "': " +
                        mkdir.error());
      }

      // Set the ownership and permissions to match the container path
      // as these are inherited from host path on bind mount.
      struct stat stat;
      if (::stat(volume.container_path().c_str(), &stat) < 0) {
        return Failure("Failed to get permissions on '" +
                        volume.container_path() + "'" +
                        ": " + strerror(errno));
      }

      Try<Nothing> chmod = os::chmod(hostPath, stat.st_mode);
      if (chmod.isError()) {
        return Failure("Failed to chmod hostPath '" +
                       hostPath +
                       "': " +
                       chmod.error());
      }

      Try<Nothing> chown = os::chown(stat.st_uid, stat.st_gid, hostPath, false);
      if (chown.isError()) {
        return Failure("Failed to chown hostPath '" +
                       hostPath +
                       "': " +
                       chown.error());
      }
    } else {
      hostPath = volume.host_path();

      if (!os::exists(hostPath)) {
        return Failure("Volume with container path '" +
                      volume.container_path() +
                      "' must have host path '" +
                      hostPath +
                      "' present on host for shared filesystem isolator");
      }
    }

    commands.push_back("mount -n --bind " +
                       hostPath +
                       " " +
                       volume.container_path());
  }

  CommandInfo command;
  command.set_value(strings::join(" && ", commands));

  return command;
}


Future<Nothing> SharedFilesystemIsolatorProcess::isolate(
    const ContainerID& containerId,
    pid_t pid)
{
  // No-op, isolation happens when unsharing the mount namespace.

  return Nothing();
}


Future<Limitation> SharedFilesystemIsolatorProcess::watch(
    const ContainerID& containerId)
{
  // No-op, for now.

  return Future<Limitation>();
}


Future<Nothing> SharedFilesystemIsolatorProcess::update(
    const ContainerID& containerId,
    const Resources& resources)
{
  // No-op, nothing enforced.

  return Nothing();
}


Future<ResourceStatistics> SharedFilesystemIsolatorProcess::usage(
    const ContainerID& containerId)
{
  // No-op, no usage gathered.

  return ResourceStatistics();
}


Future<Nothing> SharedFilesystemIsolatorProcess::cleanup(
    const ContainerID& containerId)
{
  // Cleanup of mounts is done automatically done by the kernel when
  // the mount namespace is destroyed after the last process
  // terminates.

  return Nothing();
}

} // namespace slave {
} // namespace internal {
} // namespace mesos {
