# Copyright (c) 2018 Cisco and/or its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(
  Python3
  REQUIRED
  COMPONENTS Interpreter
)

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import ply"
  RESULT_VARIABLE _rv
  OUTPUT_QUIET
)

if (NOT ${_rv} EQUAL 0)
  message( FATAL_ERROR "The \"ply\" Python3 package is not installed.")
endif()

install(
  FILES vppapigen.py
  RENAME vppapigen
  DESTINATION ${VPP_RUNTIME_DIR}
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
  COMPONENT vpp-dev
)

install(
  FILES
    vppapigen_c.py
    vppapigen_json.py
    generate_json.py
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/vpp
  COMPONENT
    vpp-dev
)
