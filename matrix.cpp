#include<stdio.h>
#include<malloc.h>
#include<stdlib.h>
#include<mpi.h>
#include<pthread.h>
#include<math.h>
#include<cstring>
int myrank, p;

// Compute C = A*B. A is a n1*n2 matrix. B is a n2*n3 matrix.
void matmul(double* A, double* B, double* C, int n1, int n2, int n3)//������˷�������ۼӵ�C������(��Ҫ��֤C�����ʼ����)
{
	int i, j, k;
	//�򵥵Ĵ��о���˷�
	for (i = 0; i < n1; i++)
	{
		for (j = 0; j < n3; j++)
		{
			for (k = 0; k < n2; k++)
			{
				C[i*n3 + j] += A[i*n2 + k] * B[k*n3 + j];
			}
		}
	}
}


int setup(int argc, char**argv, double** fstreama, double** fstreamb, int* dim)
{
	FILE* fha;
	FILE* fhb;
	int n1, n2, n3;
	int re = 1;
	if (!(fha = fopen(argv[1], "r"))) //�򿪴洢A������ļ�
	{
		printf("Can't open file %s, Errno=%d\n", argv[1], 1);//��ʧ�������Ϣ
		return -1;
	}
	if (fread(&n1, sizeof(int), 1, fha) == 0)//��ȡ���������
	{
		printf("fread error1!\n");
		return -1;
	}
	if (fread(&n2, sizeof(int), 1, fha) == 0)//��ȡ���������
	{
		printf("fread error2!\n");
		return -1;
	}
	*fstreama = (double *)malloc(n1*n2 * sizeof(double));//Ϊ���������ڴ�
	if (fread(*fstreama, sizeof(double), n1*n2, fha) == 0)//��ȡ��������
	{
		printf("fread error3!\n");
		return -1;
	}

	fclose(fha);//�رվ����ļ�

	if (!(fhb = fopen(argv[2], "r"))) //�򿪴洢A������ļ�
	{
		printf("Can't open file %s, Errno=%d\n", argv[2], 2);//��ʧ�������Ϣ
		return -1;
	}
	if (fread(&n2, sizeof(int), 1, fhb) == 0)//��ȡ���������
	{
		printf("fread error4!\n");
		return -1;
	}
	if (fread(&n3, sizeof(int), 1, fhb) == 0)//��ȡ���������
	{
		printf("fread error5!\n");
		return -1;
	}
	*fstreamb = (double *)malloc(n2*n3 * sizeof(double));//Ϊ���������ڴ�
	if (fread(*fstreamb, sizeof(double), n2*n3, fhb) == 0)//��ȡ��������
	{
		printf("fread error6!\n");
		return -1;
	}

	fclose(fhb);//�رվ����ļ�
	dim[0] = n1;//���ؾ���Ĵ�С����
	dim[1] = n2;//���ؾ���Ĵ�С����
	dim[2] = n3;//���ؾ���Ĵ�С����
	return 0;
}

void scatter_matrix(double* matrixbuf, int rows, int cols, double* local_matrix, int rootp)//�����󻮷�ΪС����鲢�ַ��������ڵ�
{
	int row, column, i, j, count;
	int maxrows_block = (rows + rootp - 1) / rootp;//СA��������������ֵ
	int maxcols_block = (cols + rootp - 1) / rootp;//С��������������ֵ
	double * matrixbuf2 = NULL;//������ʽ��ԭ����Ļ�����
	MPI_Status status;//����ͨ�ŵ�״̬

	if (myrank == 0)//0���߳�
	{
		if (!(matrixbuf2 = (double *)malloc(maxcols_block*maxrows_block*rootp*rootp * sizeof(double))))//Ϊ�����������ڴ�
		{
			printf("Memory allocation failed\n");
		}
		//������ת��Ϊ����������ŵ���ʽ������ַ�ÿ��С����ͬʱ���ڱ߽�û�ж����С���󣬲�����룬�������
		count = 0;
		for (i = 0; i < rootp; i++)
		{
			for (j = 0; j < rootp; j++)
			{
				if (i != (rootp - 1) && j == (rootp - 1))//���⴦��������һ����������һ��
				{
					for (row = 0; row < maxrows_block; row++)
					{
						for (column = 0; column < maxcols_block; column++)
						{
							if ((j * maxcols_block + column) >= cols)//�������
							{
								matrixbuf2[count] = 0;
							}
							else
							{
								matrixbuf2[count] = matrixbuf[(i * maxrows_block + row) * cols + j * maxcols_block + column];
							}
							count++;
						}
					}
				}
				else if (i == (rootp - 1) && j != (rootp - 1)) //���⴦��������һ����������һ��
				{
					for (row = 0; row < maxrows_block; row++)
					{
						for (column = 0; column < maxcols_block; column++)
						{
							if ((i * maxrows_block + row) >= rows)//�������
							{
								matrixbuf2[count] = 0;
							}
							else
							{
								matrixbuf2[count] = matrixbuf[(i * maxrows_block + row)*cols + j * maxcols_block + column];
							}
							count++;
						}
					}
				}
				else if (i == (rootp - 1) && j == (rootp - 1)) //���⴦�����һ�����һ�е��Ǹ���
				{
					for (row = 0; row < maxrows_block; row++)
					{
						for (column = 0; column < maxcols_block; column++)
						{
							if (((j * maxcols_block + column) >= cols) || ((i * maxrows_block + row) >= rows))//�������
							{
								matrixbuf2[count] = 0;
							}
							else
							{
								matrixbuf2[count] = matrixbuf[(i * maxrows_block + row) * cols + j * maxcols_block + column];
							}
							count++;
						}
					}
				}
				else  //��ͨ�Ŀ�
				{
					for (row = 0; row < maxrows_block; row++)
					{
						for (column = 0; column < maxcols_block; column++)
						{
							matrixbuf2[count] = matrixbuf[(i * maxrows_block + row)*cols + j * maxcols_block + column];
							count++;
						}
					}
				}
			}
		}
		if (count != maxcols_block*maxrows_block*rootp*rootp)//����Ƿ����
		{
			printf("scatter_matrix error!\n");
			return;
		}
		//�����ڱ��ص��Ǹ���������
		for (i = 0; i < maxrows_block*maxcols_block; i++)
		{
			local_matrix[i] = matrixbuf2[i];
		}
		//�ַ������鵽��Ӧ���߳�
		for (i = 1; i < rootp*rootp; i++)
		{
			MPI_Send((matrixbuf2 + (i * maxcols_block * maxrows_block)), maxcols_block * maxrows_block, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
		}
	}
	else    //��0���߳�
	{
		MPI_Recv(local_matrix, maxcols_block * maxrows_block, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status); //�����߳̽���0�̷߳��͵�С�����
	}
	if (matrixbuf2 != NULL) //�ͷŻ�����
	{
		free(matrixbuf2);
	}
	return;
}
//A and bufA : n1_block*n2_block;  B and bufB : n2_block*n3_block
//����cannon�㷨�������ڵ��ڱ��ؽ��о���˷�������������飬����ѭ����ֱ���������
void cannon(double* A, double* bufA, double* B, double* bufB, double* C, int n1_block, int n2_block, int n3_block, int rootp)
{
	MPI_Request send_a_req, send_b_req;
	MPI_Status send_a_status, send_b_status, recv_a_status, recv_b_status;
	int cycle_count;
	int rank_next_a, rank_next_b;
	int rank_last_a, rank_last_b;
	int curRowP, curColP, i, j;
	int tag = 0;//��ʾ��ǰ��ȷ������A�л���bufA�У�0��ʾ��A�У�1��ʾ��bufA��
				//�ȳ�ʼ�������飬��A_ijѭ������i����B_ijѭ������j����C_ij��ʼ��Ϊ��

				//��ʼ������C��ֵȫ����Ϊ0
	for (i = 0; i<n1_block; i++)
	{
		for (j = 0; j<n3_block; j++)
		{
			C[i*n3_block + j] = 0;
		}
	}

	//ѭ������С����
	curRowP = myrank / rootp;//��ǰ�ڵ�������i
	curColP = myrank%rootp;//��ǰ�ڵ�������j
	
						   //�������i����Ľڵ��
						   //���ƺ��������Ϊj - i
	if ((curColP - curRowP)<0)
	{
		rank_next_a = myrank + rootp - curRowP;
	}
	else
	{
		rank_next_a = myrank - curRowP;
	}


	//�������j����Ľڵ��
	if ((curRowP - curColP)<0)
	{
		rank_next_b = myrank - curColP*rootp + rootp*rootp;
	}
	else
	{
		rank_next_b = myrank - curColP*rootp;
	}

	//��ý�������i���Ľڵ��
	if ((curColP + curRowP) >= rootp)
	{
		rank_last_a = myrank + curRowP - rootp;
	}
	else
	{
		rank_last_a = myrank + curRowP;
	}


	//��ý�������j���Ľڵ��
	if ((curRowP + curColP) >= rootp)
	{
		rank_last_b = myrank + curColP*rootp - rootp*rootp;
	}
	else
	{
		rank_last_b = myrank + curColP*rootp;
	}

	//���������;����������Ҫ�ƶ�����ֱ�ӱ���memcpy
	if (rank_next_a != myrank)
	{
		MPI_Isend(A, n1_block*n2_block, MPI_DOUBLE, rank_next_a, 0, MPI_COMM_WORLD, &send_a_req);//���������;���A����������
	}
	else
	{
		memcpy(bufA, A, n1_block*n2_block * sizeof(double));//����ֱ��memcpy
	}
	if (rank_next_b != myrank)
	{
		MPI_Isend(B, n2_block*n3_block, MPI_DOUBLE, rank_next_b, 0, MPI_COMM_WORLD, &send_b_req);//���������;���B����������
	}
	else
	{
		memcpy(bufB, B, n2_block*n3_block * sizeof(double));//����ֱ��memcpy
	}

	//�������ܾ���
	if (rank_last_a != myrank)
	{
		MPI_Recv(bufA, n1_block*n2_block, MPI_DOUBLE, rank_last_a, 0, MPI_COMM_WORLD, &recv_a_status);//�������ܾ���A
	}
	if (rank_last_b != myrank)
	{
		MPI_Recv(bufB, n2_block*n3_block, MPI_DOUBLE, rank_last_b, 0, MPI_COMM_WORLD, &recv_b_status);//�������ܾ���B
	}

	//�����ȴ����;������
	if (rank_next_a != myrank)
	{
		MPI_Wait(&send_a_req, &send_a_status);//�������;���A������
	}
	if (rank_next_b != myrank)
	{
		MPI_Wait(&send_b_req, &send_b_status);//�������;���B������
	}

	MPI_Barrier(MPI_COMM_WORLD);//ͬ��
	tag = 1;
	if (myrank%rootp == 0)//��һ�еĽڵ�
	{
		rank_next_a = myrank + rootp - 1;
	}
	else
	{
		rank_next_a = myrank - 1;
	}

	if (myrank / rootp == 0)//��һ�еĽڵ�
	{
		rank_next_b = myrank + rootp*(rootp - 1);
	}
	else
	{
		rank_next_b = myrank - rootp;
	}

	if (myrank%rootp == (rootp - 1))//���һ�еĽڵ�
	{
		rank_last_a = myrank - rootp + 1;
	}
	else
	{
		rank_last_a = myrank + 1;
	}

	if (myrank / rootp == (rootp - 1))//���һ�еĽڵ�
	{
		rank_last_b = myrank - rootp*(rootp - 1);
	}
	else
	{
		rank_last_b = myrank + rootp;
	}
	//ѭ����ÿ������ǰ��ĳ˼����㣬��ʹ��A_ijѭ������1����B_ijѭ������1��
	for (cycle_count = 0; cycle_count < rootp; cycle_count++)
	{
		if (tag == 1)//������bufA��
		{
			matmul(bufA, bufB, C, n1_block, n2_block, n3_block);//����ǰ�ڵ�ľ���˷�
																//ѭ������С����
			MPI_Isend(bufA, n1_block*n2_block, MPI_DOUBLE, rank_next_a, 0, MPI_COMM_WORLD, &send_a_req);//���������;���A����������
			MPI_Isend(bufB, n2_block*n3_block, MPI_DOUBLE, rank_next_b, 0, MPI_COMM_WORLD, &send_b_req);//���������;���B����������

			MPI_Recv(A, n1_block*n2_block, MPI_DOUBLE, rank_last_a, 0, MPI_COMM_WORLD, &recv_a_status);//�������ܾ���A
			MPI_Recv(B, n2_block*n3_block, MPI_DOUBLE, rank_last_b, 0, MPI_COMM_WORLD, &recv_b_status);//�������ܾ���B

			MPI_Wait(&send_a_req, &send_a_status);//�������;���A������
			MPI_Wait(&send_b_req, &send_b_status);//�������;���B������
			tag = 0;
		}
		else  //������A��
		{
			matmul(A, B, C, n1_block, n2_block, n3_block);//����ǰ�ڵ�ľ���˷�
														  //ѭ������С����
			MPI_Isend(A, n1_block*n2_block, MPI_DOUBLE, rank_next_a, 0, MPI_COMM_WORLD, &send_a_req);//���������;���A����������
			MPI_Isend(B, n2_block*n3_block, MPI_DOUBLE, rank_next_b, 0, MPI_COMM_WORLD, &send_b_req);//���������;���B����������

			MPI_Recv(bufA, n1_block*n2_block, MPI_DOUBLE, rank_last_a, 0, MPI_COMM_WORLD, &recv_a_status);//�������ܾ���A
			MPI_Recv(bufB, n2_block*n3_block, MPI_DOUBLE, rank_last_b, 0, MPI_COMM_WORLD, &recv_b_status);//�������ܾ���B
			
			MPI_Wait(&send_a_req, &send_a_status);//�������;���A������
			MPI_Wait(&send_b_req, &send_b_status);//�������;���B������
			tag = 1;
		}
		MPI_Barrier(MPI_COMM_WORLD);//ͬ��
	}
	return;
}

//gather_matrix((double*)(fstreamc + sizeof(int)*2), n1, n3, C, rootp);
//�������ڵ��С����C�ռ���0�Žڵ�
void gather_matrix(double* matrixCbuf, int rows, int cols, double* local_C, int rootp, int rows_block_pad, int cols_block_pad)
{
	int curRow, curCol, i, j, curP;
	MPI_Status status;
	double * matrixC_pad = NULL;//�������ľ���C
	if (myrank == 0)  //0���߳�
	{
		if (!(matrixC_pad = (double *)malloc(rows_block_pad*cols_block_pad*rootp*rootp * sizeof(double))))//Ϊ�����������ڴ�
		{
			printf("Memory allocation failed\n");
		}
		//�����ؼ�����ֱ�Ӹ��ƹ���
		for (i = 0; i < rows_block_pad * cols_block_pad; i++)
		{
			matrixC_pad[i] = local_C[i];
		}
		//����������0�̵߳ļ�����
		for (i = 1; i < rootp*rootp; i++)
		{
			MPI_Recv(matrixC_pad + (i * rows_block_pad * cols_block_pad), rows_block_pad * cols_block_pad, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
		}

		//�����������C����ȥ����䣬������������˳��
		for (i = 0; i<rows; i++)
		{
			for (j = 0; j<cols; j++)
			{
				curP = (i / rows_block_pad)*rootp + (j / cols_block_pad);//���ڵڼ����ڵ㣬��0��ʼ
				curRow = i%rows_block_pad;//����С����ĵڼ���
				curCol = j%cols_block_pad;//����С����ĵڼ���
				matrixCbuf[i * cols + j] = matrixC_pad[curP * rows_block_pad * cols_block_pad + curRow*cols_block_pad + curCol];
			}
		}
	}
	else    //��0���߳�
	{
		MPI_Send(local_C, rows_block_pad * cols_block_pad, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);//��0���̷߳��ͼ�����
	}
	if (matrixC_pad != NULL)
	{
		free(matrixC_pad);//�ͷŻ�����
	}
	return;
}
int main(int argc, char** argv)
{
	double elapsed_time;
	// Suppose A:n1xn2, B:n2xn3. n1~n3 are read from input files
	int n1, n2, n3, rootp;
	// Buffers for matrix A, B, C. Because A, B will be shifted, so they each have two buffers
	double *A, *B, *C, *bufA, *bufB;
	// On proc 0, buffers to cache matrix files of A, B and C
	double *fstreama, *fstreamb;
	char *fstreamc;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	rootp = sqrt(p);
	if (p != rootp*rootp)
	{
		printf("Processor number must be a square!\n");
	}
	// On proc 0, preprocess the command line, read in files for A, B and
	// put their sizes in dim[].
	int dim[3];
	if (myrank == 0)  //0���̸߳�����ļ��ж�ȡ����A��B�Լ����ǵĴ�С��Ϣ
	{
		if (setup(argc, argv, &fstreama, &fstreamb, dim) != 0)
		{
			MPI_Finalize(); // Something error during preprocessing
			exit(-1);
		}
	}
	MPI_Bcast(dim, 3, MPI_INT, 0, MPI_COMM_WORLD);//0���߳̽�A��B�����size�㲥�������߳�
	n1 = dim[0];//A�� n1*n2
	n2 = dim[1];//B:  n2*n3
	n3 = dim[2];

	// Allocate memories for A, B, C, bufA and bufB.
	// Suppose an m*n matrix is 2D block-distributed on a rootp*rootp processor grid.
	// If rootp doesn't divide m or n, then submatrixes won't have the same size.
	// Because we will shift A, B, so we allocate memories according to the max
	// rows and cols of A and B.
	//��Ϊ�п���rootp��������n1,n2,n3,�����������ڴ��ʱ�������Ŀ�Ĵ�С
	int maxrows_a = (n1 + rootp - 1) / rootp;//A��������������ֵ
	int maxcols_a = (n2 + rootp - 1) / rootp;//A��������������ֵ
	int maxrows_b = maxcols_a;//B��������������ֵ
	int maxcols_b = (n3 + rootp - 1) / rootp;//B��������������ֵ
	int bufA_size = sizeof(double)*maxrows_a*maxcols_a;//��СΪһ��A�����Ĵ�С
	int bufB_size = sizeof(double)*maxrows_b*maxcols_b;//��СΪһ��B�����Ĵ�С
	int bufC_size = sizeof(double)*maxrows_a*maxcols_b;//��СΪһ��C�����Ĵ�С
	char* buf;
	int i;
	if (!(buf = (char *)malloc(bufA_size * 2 + bufB_size * 2 + bufC_size)))//��������A����飬����B����飬��һ��C�����
	{
		printf("Memory allocation failed\n");
	}
	//��������4����������ָ��λ��
	A = (double*)buf;
	bufA = (double*)(buf + bufA_size);
	B = (double*)(buf + bufA_size * 2);
	bufB = (double*)(buf + bufA_size * 2 + bufB_size);
	C = (double*)(buf + bufA_size * 2 + bufB_size * 2);
	// Proc 0 scatters A, B to other procs in a 2D block distribution fashion
	scatter_matrix((double*)fstreama, n1, n2, A, rootp);//0���̷ַ߳�A����鵽�����߳�
	MPI_Barrier(MPI_COMM_WORLD);//ͬ��
	scatter_matrix((double*)fstreamb, n2, n3, B, rootp);//0���̷ַ߳�B����鵽�����߳�
	MPI_Barrier(MPI_COMM_WORLD);//ͬ��
	elapsed_time = MPI_Wtime();//��¼���㿪ʼ��ʱ���
							   // Compute C=A*B by Cannon algorithm
	cannon(A, bufA, B, bufB, C, maxrows_a, maxcols_a, maxcols_b, rootp);
	MPI_Barrier(MPI_COMM_WORLD);//ͬ��
	elapsed_time = MPI_Wtime() - elapsed_time;//��¼�������õ�ʱ��
											  // Proc 0 gathers C from other procs and write it out
	FILE* fhc;
	int fsizec = sizeof(int) * 2 + sizeof(double)*n1*n3;//�洢C�����Լ�������С�����Ŀռ��С
	if (myrank == 0)
	{
		if (!(fhc = fopen(argv[3], "w"))) //�����C������ļ�
		{
			printf("Can't open file %s, Errno=%d\n", argv[3], 3);//��ʧ�������Ϣ
			MPI_Finalize();
		}
		fstreamc = (char *)malloc(fsizec);//����洢����C���ڴ�ռ�
		((int*)fstreamc)[0] = n1;//��¼����C������
		((int*)fstreamc)[1] = n3;//��¼����C������
	}
	gather_matrix((double*)(fstreamc + sizeof(int) * 2), n1, n3, C, rootp, maxrows_a, maxcols_b);//�ۼ��������������߳̽��Լ���C����鷢�͸��߳�0
	MPI_Barrier(MPI_COMM_WORLD); // Make sure proc 0 read all it needs
	if (myrank == 0)
	{
		printf("Cannon algrithm: multiply a %dx%d with a %dx%d, use %.2f(s)\n", n1, n2, n2, n3, elapsed_time);
		fwrite(fstreamc, sizeof(char), fsizec, fhc);//�߳�0������Cд���ļ�
		fclose(fhc);//�ر��ļ�
		free(fstreama);//�ͷ��ڴ�
		free(fstreamb);//�ͷ��ڴ�
		free(fstreamc);//�ͷ��ڴ�
	}
	free(buf);//�ͷŴ洢С�������ڴ�ռ�
	MPI_Finalize();
	return 0;
}
