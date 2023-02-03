#include "elba.h"
#include "elba_error.h"
#include <unistd.h>

void elba_opt_init(elba_opt_t *opt)
{
    opt->kmer_size = 31;
    opt->lower_kmer_bound = 15;
    opt->upper_kmer_bound = 35;
    opt->xdrop = 15;
    opt->mat = 1;
    opt->mis = -1;
    opt->gap = -1;
    opt->target_fname = NULL;
    opt->query_fname = NULL;
    opt->output_fname = "out.paf";
}

int elba_opt_parse(elba_opt_t *opt, int myrank, int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "k:L:U:x:A:B:G:o:h")) >= 0)
    {
        if (c == 'k') opt->kmer_size = atoi(optarg);
        else if (c == 'L') opt->lower_kmer_bound = atoi(optarg);
        else if (c == 'U') opt->upper_kmer_bound = atoi(optarg);
        else if (c == 'x') opt->xdrop = atoi(optarg);
        else if (c == 'A') opt->mat = atoi(optarg);
        else if (c == 'B') opt->mis = -atoi(optarg);
        else if (c == 'G') opt->gap = -atoi(optarg);
        else if (c == 'o') opt->output_fname = optarg;
        else if (c == 'h') return elba_opt_usage(*opt, myrank, argv[0]);
    }

    if (optind + 2 != argc)
    {
        if (myrank == 0)
            fprintf(stderr, "error: missing FASTA argument(s)\n");

        return elba_opt_usage(*opt, myrank, argv[0]);
    }

    opt->target_fname = argv[optind++];
    opt->query_fname = argv[optind];

    return ELBA_SUCCESS;
}

int elba_opt_usage(const elba_opt_t opt, int myrank, const char *prog)
{
    if (myrank == 0)
    {
        fprintf(stderr, "Usage: %s [options] <target.fa> <query.fa>\n", prog);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "    -k INT   k-mer size [%d]\n", opt.kmer_size);
        fprintf(stderr, "    -L INT   lower k-mer frequency bound [%d]\n", opt.lower_kmer_bound);
        fprintf(stderr, "    -U INT   upper k-mer frequency bound [%d]\n", opt.upper_kmer_bound);
        fprintf(stderr, "    -x INT   x-drop alignment threshold [%d]\n", opt.xdrop);
        fprintf(stderr, "    -A INT   matching score [%d]\n", opt.mat);
        fprintf(stderr, "    -B INT   mismatch penalty [%d]\n", -opt.mis);
        fprintf(stderr, "    -G INT   gap penalty [%d]\n", -opt.gap);
        fprintf(stderr, "    -o FILE  output PAF filename [%s]\n", opt.output_fname);
        fprintf(stderr, "    -h       help message\n");
    }

    return ELBA_TERMINATE;
}

int elba_opt_log(elba_opt_t const opt, int myrank, FILE *f)
{
    if (!myrank)
    {
        fprintf(f, "=============== input/output files ===============\n");
        fprintf(f, "'%s' (target FASTA)\n", opt.target_fname);
        fprintf(f, "'%s' (query FASTA)\n", opt.query_fname);
        fprintf(f, "'%s' (output PAF)\n", opt.output_fname);
        fputc('\n', f);

        fprintf(f, "=========== k-mer counting parameters ============\n");
        fprintf(f, "k=%d (k-mer size)\n", opt.kmer_size);
        fprintf(f, "L=%d (lower k-mer frequency bound)\n", opt.lower_kmer_bound);
        fprintf(f, "U=%d (upper k-mer frequency bound)\n", opt.upper_kmer_bound);
        fputc('\n', f);

        fprintf(f, "=========== x-drop alignment parameters ==========\n");
        fprintf(f, "x=%d (x-drop alignment threshold)\n", opt.xdrop);
        fprintf(f, "A=%d (matching score)\n", opt.mat);
        fprintf(f, "B=%d (mismatch penalty)\n", opt.mis);
        fprintf(f, "G=%d (gap penalty)\n", opt.gap);
        fputc('\n', f);
    }

    return ELBA_SUCCESS;
}

