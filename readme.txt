# To compile

ml gcc/7.1.0 
mpicc -DNCX_PTR_SIZE=8 -pipe  -O3  -DLOG_LEVEL=4  -DPAGE_MERGE  -o read_remote_file read_remote_file.c dict.c xxhash.c ncx_slab.c

gcc -O2 -o prep_file prep_file.c


# To run your job, 

0. Prepare necessary file with find then run prep
prep n_partition list [bcast_dir]

ml cuda/9.0 cudnn/7.0
cd /work/00410/huang/maverick2/test/examples

1. Start file server
export TMPFS_ROOT=/tmp/tmpfs_`id -u`
export DIR_BCAST=/tmp/tmpfs_`id -u`/ILSVRC2012_img_val/

unset LD_PRELOAD
mpiexec.hydra -f hostfile -np 4 -ppn 1 /full_path/read_remote_file 4 /work/00946/zzhang/imagenet/48-partitions-tmpfs 1

2. Sanity check
export LD_PRELOAD=/full_path/wrapper.so
mpiexec.hydra -f hostfile -np 4 -ppn 1 /home1/00410/huang/tools/tmpfs/read_file_mpi /work/00946/zzhang/imagenet/16-parts-test-horovod/flist-train.file /tmp/tmpfs_`id -u`

Example output, 
Rank 3, Time 12363237 us. 9160237224 bytes. Speed  740.9 MB/s.
Rank 0, Time 12715851 us. 9160237224 bytes. Speed  720.4 MB/s.
Rank 1, Time 12915532 us. 9160237224 bytes. Speed  709.2 MB/s.
Rank 2, Time 13238308 us. 9160237224 bytes. Speed  691.9 MB/s.

3. Test
mpiexec.hydra -f hostfile -np 16 -ppn 4 python keras_imagenet_resnet50_test.py |& tee log_n4_ppn4_w6_0

