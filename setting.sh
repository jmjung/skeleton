#!bash
mount -t tmpfs /group;
su;
mount -t cgroup none /home/jmjung/cgroup -o memory;
mkdir /home/jmjung/cgroup/group;
mkdir /home/jmjung/cgroup/group/g1;
mkdir /home/jmjung/cgroup/group/g2;
mkdir /home/jmjung/cgroup/group/g3;
mkdir /home/jmjung/cgroup/group/g4;
echo 1024M > /home/jmjung/cgroup/group/memory.limit_in_bytes;
echo 512M > /home/jmjung/cgroup/group/g1/memory.limit_in_bytes;
echo 512M > /home/jmjung/cgroup/group/g2/memory.limit_in_bytes;
echo 200M > /home/jmjung/cgroup/group/g3/memory.limit_in_bytes;
echo 824M > /home/jmjung/cgroup/group/g4/memory.limit_in_bytes;
