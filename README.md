project1
========
project4

Got a very odd bug - Server initializes just fine, but Client, when going through the exact same process, blocks during the retransmission of the ack phase and the V in minithread(sleep), while on the correct minithread (checked the pointers in gdb), does not succesfully wake it back up.

I sat with a TA for several hours and we could not figure it out :(.  Been continuing to debug and exploring many possible explanations for the bug but no success. I hate submitting broken code.


