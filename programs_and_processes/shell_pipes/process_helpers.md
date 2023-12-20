See the total resident set size (amt of physical memory)
ps -p 8770 -o rss

process tree
ps axjf

See memory blocks being used by a process
(Didn't help much, wasn't sure what I was looking at!)
pmap -x <pid>