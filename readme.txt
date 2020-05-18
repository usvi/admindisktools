Various small C programs for administrating disks on your system.
Strong pre-alpha quality. If you unwantingly destroy your data
with these, its not my fault.





diskcont
The original program which started it all. Writes
running number to all of the disk and reads it all back. If the
read blocks are continuous, disk is at least somewhat good.
Displays also current speed of operation.

Syntax:
diskcont [<parameters>] /path/to/disk

Parameters (cannot (yet) be combined like -wrs):
-w : Write only part of test
-r : Read only part of the test
-s : Silent, don't ask for confirmation (never use this)

Examples:
Make full rw test on /dev/sdx (need to confirm):
diskcont /dev/sdx

Make write only test on /dev/sdx, but without confirmation:
diskcont -w -s /dev/sdx





diskinfo
Reads disk information: model, serial, firmware and size.

Syntax:
diskinfo /path/to/disk

Examples:
Read information of /dev/sdb:
diskinfo /dev/sdb





raidkill
Needed to make this while debugging my QNAP. It seemed the NAS was
forcifully putting back the disks in raid mode. This brute command
should kill any raids there are. Raid implementations
unfortunately differ, so we just basically write a bunch of zeros
to beginning and end of device. And fsync it of course. Verifying
happens by reading back the size and comparing to buffer full of
zeros.

Syntax:
raidkill [<parameters>] /path/to/disk

Parameters (cannot (yet) be combined like -wrs):
-w : Kill the raid by writing data to beginning and end
-r : Verify that beginning and end positions are empty
-s : Silent, don't ask for confirmation (never use this)

Examples:
Kill raid on /dev/sdx and verify it (need to confirm):
raidkill /dev/sdx

Kill only raid on /dev/sdx, but without confirmation:
diskcont -w -s /dev/sdx
