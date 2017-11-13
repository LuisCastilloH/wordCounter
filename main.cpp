// Luis Castillo

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <fstream>
#include <Windows.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>
#include <tchar.h>

#define MAXP 1000
int amountOfThreads;
int results[100];
int dataPerThread;

using namespace std;

char patron[MAXP];
int rep = 0;            // number of repetitions

// ----------- copy&paste of MSDN
typedef BOOL (WINAPI
*LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG bitMask) {
	DWORD LSHIFT = sizeof(ULONG *)*8 - 1;
	DWORD bitSetCount = 0;
	ULONG bitTest = (1 << LSHIFT);
	DWORD i;
	for (i = 0; i <= LSHIFT; ++i) {
	if (bitMask & bitTest)
	bitSetCount++;
	bitTest/=2;
	}
	return bitSetCount;
}
// ----------------------------------------

struct _interval {
	char *text;
	char *keyword;
	int n;
	int m;
	int numOfThread;
	int *bmBC;
};

int getCores();

// Thread routine
DWORD WINAPI myThreadRoutine(LPVOID p) {
	struct _interval *interval = (struct _interval *)p;
	int localReps = 0;
	int startOffset = interval->numOfThread*dataPerThread;
	int endOffset = startOffset+dataPerThread;
	int i = 0;
	int m = interval->m;
	int n = interval->n;
	char c;

	if(interval->numOfThread == amountOfThreads-1)
		endOffset = endOffset + dataPerThread % amountOfThreads - m;

	i = startOffset;
	while(i<endOffset)
    {
		c = interval->text[i + m-1];
		if ( interval->keyword[m - 1] == c && memcmp(interval->keyword, interval->text + i, m - 1) == 0 )
        {
            // cout<<" * Encontrado en : "<< i + 1 << endl; 
			results[interval->numOfThread]++;
		}
		i = i + interval->bmBC[c];     
    }
	return 0;
}

/******************** Text file to string ****************************/
string get_file_contents(const char *filename) {
	ifstream in(filename, ios::in | ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

// preprocessing
void preBMH(char *P, int m, int table[])
{
	int i;
	for(i=0; i<256; i++)
		table[i] = m;
	for(i=0; i<m-1; i++)
		table[P[i]] = m-i-1;
}

// function to call Boyer Moore Horspool
void BMH( char *T, int n , char *P, int m)
{
    int  i, j, bmBC[256];
    char c;
	clock_t start, finish;
	HANDLE *th;
	struct _interval *threadData;
	// amountOfThreads = getCores();
	amountOfThreads = 8;
	threadData = (struct _interval *)malloc(amountOfThreads*sizeof(struct _interval));
	th = (HANDLE *)malloc(amountOfThreads*sizeof(HANDLE));
	dataPerThread = n/amountOfThreads;
    preBMH(P, m, bmBC) ;  // Preprocessing 

    // Creation of threads
	start = clock();
	for(i=0; i<amountOfThreads; i++) {
		threadData[i].text = T;
		threadData[i].keyword = P;
		threadData[i].n = n;
		threadData[i].m = m;
		threadData[i].numOfThread = i;
		threadData[i].bmBC = bmBC;
		th[i] = CreateThread(NULL, 0, &myThreadRoutine, &threadData[i], 0, NULL);
	}
	WaitForMultipleObjects(amountOfThreads, &th[0], TRUE, INFINITE);
	finish = clock();
	cout << " >> Tiempo de busqueda : " << finish-start << endl;

	for(i = 0; i<amountOfThreads; i++) {
        rep += results[i];
        cout << results[i];
    }
    if(rep == 0)
        cout << " >> Patron no encontrado ..! " << endl;
    else
        cout << " >> Ocurrencias : " << rep << endl;
}

/*************************** Main loop *****************************/
int main()
{
    float t1 , t2, tiempo;
    char *fileName = "big.txt";
	string str = get_file_contents(fileName);
	int dataLength = str.size();
	char *data = new char[str.size() + 1];
	std::copy(str.begin(), str.end(), data);
	data[str.size()] = '\0';
    cout << endl << " Insert keyword : \n\t\t";
    gets(patron);
    int n = strlen(data); 
    int m = strlen(patron);
    
    BMH(data, n, patron, m);

    return 0;
}

//------------------------- copy&paste of MSDN
int getCores()
{
    LPFN_GLPI glpi;
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

    glpi = (LPFN_GLPI) GetProcAddress(
                            GetModuleHandle(TEXT("kernel32")),
                            "GetLogicalProcessorInformation");
    if (NULL == glpi)
    {
        _tprintf(TEXT("\nGetLogicalProcessorInformation is not supported.\n"));
        return (1);
    }

    while (!done)
    {
        DWORD rc = glpi(buffer, &returnLength);

        if (FALSE == rc)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (buffer)
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                        returnLength);

                if (NULL == buffer)
                {
                    _tprintf(TEXT("\nError: Allocation failure\n"));
                    return (2);
                }
            }
            else
            {
                _tprintf(TEXT("\nError %d\n"), GetLastError());
                return (3);
            }
        }
        else
        {
            done = TRUE;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
        switch (ptr->Relationship)
        {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
            processorPackageCount++;
            break;

        default:
            //_tprintf(TEXT("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n"));
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
    _tprintf(TEXT("Number of processor cores: %d\n"),
             processorCoreCount);

    free(buffer);

    return processorCoreCount;
}
// -------------------------------