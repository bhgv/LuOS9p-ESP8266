/* Only the Styx types/fields accessed by the extracted RPC read callback. */
#define FS_RPC 1
#define nil NULL
typedef unsigned long ulong;
typedef long long vlong;
typedef void Styxfile;
typedef struct Qid Qid;
struct Qid {
    char *my_buf;
    char *my_name;
    int my_buf_idx;
    int my_state;
    int my_type;
};
char *fsrpcread(Qid *, char *, ulong *, vlong *);
