#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define PATH_LEN 100
#define MAX 20
#define MAX_LINE 50

void setup_env();
int read_files(char *, char *, char *);
void rows_cols_no(FILE *, int);
void create_matrices(FILE *, int);
void thread_per_mat(char *);
void thread_per_row(char *);
void thread_per_element(char *);
void *compute_per_row(void *);
void *compute_per_elmnt(void *);


struct args{  //will be used to pass multiple args to the threading function
    int arow;
    int bcol;
};


int rows1, cols1, rows2, cols2, a[MAX][MAX], b[MAX][MAX], c[MAX][MAX];
FILE *fpin1, *fpin2, *fpout1, *fpout2, *fpout3, *fpall;
struct timeval stop, start;  // will be used to calculate the runtime of the function


int main(int argc, char *argv[])
{
    setup_env(); //change to the current working directory

    char *in1, *in2, *out;
    if(argc == 1){      // check the number of args
        in1 = "a"; in2 = "b"; out = "c";  //defaults if no args are given
    }
    else{
        in1 = argv[1];
        in2 = argv[2];
        out = argv[3];
    }
    read_files(in1, in2, out);  // read matrices from the txt files

    thread_per_mat(out);
    thread_per_row(out);
    thread_per_element(out);

    return 0;
}


void setup_env()
{
    char path[PATH_LEN]; //to store the path
    chdir(getcwd(path, PATH_LEN));
}

int read_files(char *in1, char *in2, char *out)  //initialzie the ip files and create the op files
{
    char out_file[255];
    char *thread_types[3] = {"_per_matrix", "_per_row", "_per_element"};
    FILE *fpout_pointers[3];  //array of FILE pointers

    for(char i = 0; i <= 2; i++){
        strcpy(out_file, out);  // append the name of the output file to out_file
        strcat(out_file, thread_types[i]);	//append the type of threading to the name of the file
        fpall = fopen(out_file, "w");
        if(!fpall){ perror("File not open"); return 0; }
        fpout_pointers[i] = fpall;
    }

    fpin1 = fopen(in1, "r");  //matrix 1
    fpin2 = fopen(in2, "r");  //matrix 2
    fpout1 = fpout_pointers[0];  //per_matrix
    fpout2 = fpout_pointers[1];  //per_row
    fpout3 = fpout_pointers[2];  //per_element

    if(!fpin1 || !fpin2){
        perror("File not open");
        return 0;
    }

    rows_cols_no(fpin1, 1);  //read the number of rows and cols in matrix 1
    rows_cols_no(fpin2, 2);  //read the number of rows and cols in matrix 2

    if(cols1 != rows2){ //make sure that the multiplication process is valid
        printf("Number of rows of the first matrix must be equal the number of columns of the second matrix\n");
        exit(0);
    }

    create_matrices(fpin1, 1);  //create the matrix stored in the first file
    create_matrices(fpin2, 2);

    fclose(fpin1);  fclose(fpin2);
    fclose(fpout1); fclose(fpout2); fclose(fpout3);  //files that contain the solution
}


void rows_cols_no(FILE *fp, int which_mat)  //read the number of rows and cols
{
    char line[MAX_LINE];
    fgets(line, sizeof(line), fp);  //store the first line in the txt file in 'line'

    if(which_mat == 1)
        sscanf(line, "row=%d col=%d", &rows1, &cols1);
    else
        sscanf(line, "row=%d col=%d", &rows2, &cols2);
}


void create_matrices(FILE *fp, int which_mat)
{
    if(which_mat == 1){  //read matrix 1
        for(int i = 0; i < rows1; ++i){
            for(int j = 0; j < cols1; ++j)
                fscanf(fp, "%d", a[i] + j);
        }
    }
    else{  //read matrix 2
        for(int i = 0; i < rows2; ++i){
            for(int j = 0; j < cols2; ++j)
                fscanf(fp, "%d", b[i] + j);
        }
    }
}


void thread_per_mat(char* out)
{
    int sum = 0, i, j, k;
    char per_mat_fname[20];  // to setup the name of the output file
    strcat(per_mat_fname, out);
    strcat(per_mat_fname, "_per_matrix");
    fpout1 = fopen(per_mat_fname, "w");

    gettimeofday(&start, NULL); //start checking time

    for(i = 0; i < rows1; i++){  //compute
        for(j = 0; j < cols2; j++){
            for(k = 0; k < rows2; k++){
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
            fprintf(fpout1, "%d ", c[i][j]);
            sum = 0;
        }
        fprintf(fpout1, "\n");
    }

    gettimeofday(&stop, NULL); //end checking time
    fprintf(fpout1, "\n\nExecution time: %ld sec. %ld usec", stop.tv_sec - start.tv_sec, stop.tv_usec - start.tv_usec);

    fclose(fpout1);
}


void thread_per_row(char *out)
{
    char per_row_fname[20] = "";  // to setup the name of the output file
    int i, j;
    pthread_t tid[rows1];

    strcat(per_row_fname, out);
    strcat(per_row_fname, "_per_row");
    fpout2 = fopen(per_row_fname, "w");

    gettimeofday(&start, NULL); //start checking time

    for(i = 0; i < rows1; i++)
        pthread_create(&tid[i], NULL, compute_per_row, (void *)&i);

    for(i = 0; i < rows1; i++)
        pthread_join(tid[i], NULL);

    gettimeofday(&stop, NULL); //end checking time

    for (i = 0; i < rows1; i++){
        for(j = 0; j < cols2; j++){
            fprintf(fpout2, "%d ", c[i][j]);
        }
        fprintf(fpout2, "\n");
    }

    fprintf(fpout2, "\n\nExecution time: %ld sec. %ld usec", stop.tv_sec - start.tv_sec, stop.tv_usec - start.tv_usec);

    fclose(fpout2);
}


void thread_per_element(char *out)
{
    char per_elmnt_fname[20] = "";  // to setup the name of the output file
    int i, j;
    pthread_t tid[rows1][cols2];
    struct args args_array[rows1][cols2];  //will be used to pass multiple arguments to the compute_per_elmnt func.

    strcat(per_elmnt_fname, out);
    strcat(per_elmnt_fname, "_per_element");
    fpout3 = fopen(per_elmnt_fname, "w");

    gettimeofday(&start, NULL); //start checking time

    for(i = 0; i < rows1; i++){
        for(j = 0; j < cols2; j++){
            args_array[i][j].arow = i;
            args_array[i][j].bcol = j;
            pthread_create(&tid[i][j], NULL, compute_per_elmnt, (void *)&args_array[i][j]);
        }
    }

    for(i = 0; i < rows1; i++){
        for(j = 0; j < cols2; j++)
            pthread_join(tid[i][j], NULL);
    }

    gettimeofday(&stop, NULL); //end checking time

    for(i = 0 ; i < rows1; i++){
        for(j = 0; j < cols2; j++)
            fprintf(fpout3, "%d ", c[i][j]);
        fprintf(fpout3, "\n");
    }

    fprintf(fpout3, "\n\nExecution time: %ld sec. %ld usec", stop.tv_sec - start.tv_sec, stop.tv_usec - start.tv_usec);

    fclose(fpout3);
}


void *compute_per_row(void *ptr)
{
    int idx, sum, i, j;
    idx = *(int *)ptr;

    for(i = 0; i < cols1; i++){
        sum = 0;
        for(j = 0; j < rows2; j++){
            sum += a[idx][j] * b[j][i];
        }
        c[idx][i] = sum;
    }

    pthread_exit(NULL);
}


void *compute_per_elmnt(void *ptr)
{
    struct args *row_col_no = (struct args *)ptr;
    int row = row_col_no -> arow;
    int col = row_col_no -> bcol;
    int sum = 0;

    for(int k = 0; k < cols1; k++){
        sum += a[row][k] * b[k][col];
    }
    c[row][col] = sum;

    pthread_exit(NULL);
}
