#!/usr/bin/env python
# Icinga 2
# Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

# This script assumes that you have the following templates:
#
# template Host "agent-host" {
#   check_command = "agent"
#   vars.agent_host = "$address$"
#   vars.agent_port = 7000
# }
#
# template Service "agent-service" {
#   check_command = "agent"
#}

import subprocess, json

inventory_json = subprocess.check_output(["icinga2-list-agents", "--batch"])
inventory = json.loads(inventory_json)

for host, hostinfo in inventory.items():
    print "object Host \"%s\" {" % (host)
    print "  import \"agent-host\""

    if "peer" in hostinfo:
        print "  vars.agent_host = \"%s\"" % (hostinfo["peer"]["agent_host"])
        print "  vars.agent_host = \"%s\"" % (hostinfo["peer"]["agent_port"])

    print "}"
    print ""

    for service in hostinfo["services"]:
        print "object Service \"%s\" {" % (service)
        print "  import \"agent-service\""
        print "  host_name = \"%s\"" % (host)
        print "}"
        print ""
