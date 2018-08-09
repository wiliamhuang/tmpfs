# To compile

ml gcc/7.1.0 
mpicc -DNCX_PTR_SIZE=8 -pipe  -O3  -DLOG_LEVEL=4  -DPAGE_MERGE  -o read_remote_file read_remote_file.c dict.c xxhash.c ncx_slab.c

gcc -O2 -o prep_file prep_file.c


# To run your job, 
0. Prepare necessary file with find then prep 
prep n_partition list 
