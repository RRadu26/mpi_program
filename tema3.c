#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printtop(int rank, int si, int topology[4][si]);
int main(int argc, char* argv[]) {
    char file_name[20];
    FILE *file;

    // Get the number of processes and the rank of this process
    MPI_Init(&argc, &argv);
    int num_procs, rank, cluster_size, si, dec;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    si = num_procs - 4;
    MPI_Status status;
    int topology[4][si];
    int err = atoi(argv[2]);
    if(err == 2)
        return 0;
    //cod specific coordonatorilor
    if (rank < 4) {
        int np;
        //v contine vectorul de numere pt operatiile workerilor.
        int *V;
        //nu este numarul de operatii pe care trebuie sa le faca un worker, iar nt este numarul de operatii ce trebuie
        //facut de clusterul urmator(de exemplu in procesul 0 este 1, in 1 este 2, etc. Se opreste in 3)
        int nu, nt, nramas;
        int topologyc[si];
        //fiecare proces citeste clusterul sau de procese subordonate worker. Numarul lor este tinut in cluster_size
        //iar topologia este tinuma in topology[rank]. Topologia finala a programului este in topology
        sprintf(file_name, "cluster%d.txt", rank);
        file = fopen(file_name, "r");
        if (file == NULL) {
            printf("Error opening file %s\n", file_name);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fscanf(file, "%d", &cluster_size);

        for (int i = 0; i < cluster_size; i++) {
            fscanf(file, "%d", &topology[rank][i]);
        }
        //deoarece topology[rank] are mereu num_procs - 4, cand topologia este completa se pune -1 dupa ultimul proces din cluster
        if(cluster_size < num_procs - 5)
            topology[rank][cluster_size] = -1;
        fclose(file);


        if(rank != 0 && rank != 2 && err == 0) {
            //procesele 1 si 3 trimit topologiile lor la procesul 0
            MPI_Send(&topology[rank], si, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        } else if(rank == 3) {
            MPI_Send(&topology[rank], si, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        } else if(rank == 1) {
            MPI_Send(&topology[rank], si, MPI_INT, 2, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

        }

        if(rank == 2) {
            //procesul 2 ii trimite procesului 3 topologia sa
            MPI_Send(topology[rank], si, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            if(err == 1) {
                MPI_Recv(topologyc, si, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
                MPI_Send(topologyc, si, MPI_INT, 3, 1, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 3);
           }
        }

        if(rank == 3) {
            //procesul 3 primeste si apoi trimite catre 0 topologia procesului 2
            MPI_Recv(topologyc, si, MPI_INT, 2, 2, MPI_COMM_WORLD, &status);
            MPI_Send(topologyc, si, MPI_INT, 0, 2, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            if(err == 1) {
                MPI_Recv(topologyc, si, MPI_INT, 2, 1, MPI_COMM_WORLD, &status);
                MPI_Send(topologyc, si, MPI_INT, 0, 1, MPI_COMM_WORLD); 
                printf("M(%d,%d)\n", rank, 0);            
            }

        }
        if(rank == 0) {
            //primeste topologiile. Topologia procesului 2 va fi trimisa de 3.
            if(err == 0)
                MPI_Recv(topology[1], si, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
            else
                MPI_Recv(topology[1], si, MPI_INT, 3, 1, MPI_COMM_WORLD, &status);

            MPI_Recv(topology[3], si, MPI_INT, 3, 3, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], si, MPI_INT, 3, 2, MPI_COMM_WORLD, &status);
            printtop(rank, si, topology);

            //trimite topologia completa procesului 3
            MPI_Send(topology, 4 * si, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            int N = atoi(argv[1]);
            V = malloc(sizeof(int) * N);
            for (int i = 0; i < N; i++) {
                V[i] = N - i - 1;
            }
            nu = N / si;
            nramas = N % si;
            dec = (nramas < cluster_size) ? nramas : cluster_size;  
            nramas -= dec;         
            nt = N - nu * cluster_size - dec;

            //se trimite catre procesul 3 numarul de operatii pe worker
            MPI_Send(&nu, 1, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            //se trimite catre procesul 3 cate numere va primi
            MPI_Send(&nt, 1, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            //se trimit efectiv numerele
            MPI_Send(V + (nu*cluster_size + dec), nt, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            //se trimit numerele ramase
            MPI_Send(&nramas, 1, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

        } else {
            //cele 3 procese coordonator primesc topologia completa. Se incepe cu 3, care
            //trimite la 2, etc.
            if(rank != 3)
                MPI_Recv(topology, 4 * si, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &status);
            else
                MPI_Recv(topology, 4 * si, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            printtop(rank, si, topology);

            if(rank != 1) {
                //fiecare proces trimite procesului anterior topologia completa, in afara de 1
                MPI_Send(topology, 4 * si, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
            }
            if(rank != 3) {
            //se primesc parametrii specifici operatiilor workerilor si vectorul
            MPI_Recv(&nu, 1, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&nt, 1, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
            np = nt;
            V = malloc(sizeof(int) * nt);
            MPI_Recv(V, nt, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&nramas, 1, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(&nu, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&nt, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
                np = nt;
                V = malloc(sizeof(int) * nt);
                MPI_Recv(V, nt, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&nramas, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);                
            }
            dec = (nramas < cluster_size) ? nramas : cluster_size;           

            if(rank != 1) {
                //se trimit mai departe parametrii.
                MPI_Send(&nu, 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank  - 1);

                nt = nt - nu * cluster_size;

                MPI_Send(&nt, 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank  - 1);

                MPI_Send(V + (nu*cluster_size + dec ), nt, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);       
                nramas = nramas - dec;
                MPI_Send(&nramas, 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
            }
        }

        np = (np < cluster_size) ? np : cluster_size;
        int wts =0;
        int *fin_numbers;
        int size_fn = nu*cluster_size + dec;
        fin_numbers = malloc(sizeof(int)* (size_fn));
        for(int i = 0 ; i < cluster_size; i++) {
            int nuinloc = nu;
            //coordonatorii trimit "copiilor" rankul lor, topologia completa si parametrii specifici operatiilor copilului
            MPI_Send(&rank, 1, MPI_INT, topology[rank][i], 20, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, topology[rank][i]);

            MPI_Send(topology, 4 * si, MPI_INT, topology[rank][i], rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, topology[rank][i]);
            int zero = 0;
            if(np <= 0) {
                MPI_Send(&zero, 1, MPI_INT, topology[rank][i], rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, topology[rank][i]);    
            }
            else {
                if(dec > 0) {
                    nuinloc += 1;
                    dec--;
                }
                MPI_Send(&nuinloc, 1, MPI_INT, topology[rank][i], rank, MPI_COMM_WORLD); 
                printf("M(%d,%d)\n", rank, topology[rank][i]);               
                MPI_Send(V + wts, nuinloc, MPI_INT, topology[rank][i], rank, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, topology[rank][i]);
                MPI_Recv(fin_numbers + wts, nuinloc, MPI_INT, topology[rank][i], topology[rank][i], MPI_COMM_WORLD, &status);
                wts += nuinloc;

                np--;
            }
        }
        if(rank == 1 && err == 0) {
            MPI_Send(&size_fn, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(fin_numbers, size_fn, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        } else if(rank == 1) {
            MPI_Send(&size_fn, 1, MPI_INT, 2, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(fin_numbers, size_fn, MPI_INT, 2, rank, MPI_COMM_WORLD);    
            printf("M(%d,%d)\n", rank, 2);        
        }

        if(rank == 2) {
            MPI_Send(&size_fn, 1, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(fin_numbers, size_fn, MPI_INT, 3, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            if(err == 1) {
                int nsizefin1;
                MPI_Recv(&nsizefin1, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
                int fin1[nsizefin1];
                MPI_Recv(fin1, nsizefin1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);

                MPI_Send(&nsizefin1, 1, MPI_INT, 3, 1, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 3);
                MPI_Send(fin1, nsizefin1, MPI_INT, 3, 1, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 3);
            }
        }

        if(rank == 3) {
            int nsizefin2;
            MPI_Recv(&nsizefin2, 1, MPI_INT, 2, 2, MPI_COMM_WORLD, &status);
            int fin2[nsizefin2];
            MPI_Recv(fin2, nsizefin2, MPI_INT, 2, 2, MPI_COMM_WORLD, &status);

            MPI_Send(&nsizefin2, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(fin2, nsizefin2, MPI_INT, 0, 2, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(&size_fn, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(fin_numbers, size_fn, MPI_INT, 0, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            if(err == 1) {
                int nsizefin1;
                MPI_Recv(&nsizefin1, 1, MPI_INT, 2, 1, MPI_COMM_WORLD, &status);
                int fin1[nsizefin1];
                MPI_Recv(fin1, nsizefin1, MPI_INT, 2, 1, MPI_COMM_WORLD, &status);

                MPI_Send(&nsizefin1, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);

                MPI_Send(fin1, nsizefin1, MPI_INT, 0, 1, MPI_COMM_WORLD);               
                printf("M(%d,%d)\n", rank, 0);
            }
        }

        if(rank == 0) {
            int V[atoi(argv[1])];
            memcpy(V, fin_numbers, size_fn * sizeof(int));
            int *Vc = V + size_fn;
            int size1, size2, size3;
            if(err == 0)
                MPI_Recv(&size1, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
            else 
                MPI_Recv(&size1, 1, MPI_INT, 3, 1, MPI_COMM_WORLD, &status);


            MPI_Recv(&size2, 1, MPI_INT, 3, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&size3, 1, MPI_INT, 3, 3, MPI_COMM_WORLD, &status);

            MPI_Recv(Vc, size3, MPI_INT, 3, 3, MPI_COMM_WORLD, &status);
            Vc += size3;

            MPI_Recv(Vc, size2, MPI_INT, 3, 2, MPI_COMM_WORLD, &status);
            Vc += size2;
            
            if(err == 0)
                MPI_Recv(Vc , size1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
            else
                MPI_Recv(Vc , size1, MPI_INT, 3, 1, MPI_COMM_WORLD, &status);
            printf("Rezultat: ");
            for(int i = 0; i < atoi(argv[1]); i++)
                printf("%d ", V[i]);
        }
    //cod specific workerilor
    } else {
        int no;
        //se primesc date de la procesul coordonator specific
        int father;
        MPI_Recv(&father, 1, MPI_INT, MPI_ANY_SOURCE, 20, MPI_COMM_WORLD, &status);
        MPI_Recv(topology, 4 * si, MPI_INT, father, father, MPI_COMM_WORLD, &status);
        printtop(rank, si, topology);
        MPI_Recv(&no, 1, MPI_INT, father, father, MPI_COMM_WORLD, &status);
        if(no != 0) {
            int V[no];
            MPI_Recv(V, no, MPI_INT, father, father, MPI_COMM_WORLD, &status);
            for(int i = 0 ; i < no; i++)
                V[i] *= 5;
            MPI_Send(V, no, MPI_INT, father, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, father);
        }
    }
    MPI_Finalize();
}

void printtop(int rank, int si, int topology[4][si]) {
    printf("%d ->", rank);
    for(int i = 0; i < 4 ; i ++) {
        printf(" %d:", i);
            for(int j = 0 ; j < si; j++){
                if(topology[i][j] == -1)
                    break;
            printf("%d", topology[i][j]);
            if(topology[i][j+1] != -1)
                printf(",");
    }
}
    printf( "\n");
}