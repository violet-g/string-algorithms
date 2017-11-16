#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// global variables
enum {LCS, ED, SW, NONE} alg_type; // which algorithm to run
char *alg_desc; // description of which algorithm to run
char *result_string; // text to print along with result from algorithm
char *x, *y; // the two strings that the algorithm will execute on
char *filename; // file containing the two strings
int xLen, yLen, alphabetSize; // lengths of two strings and size of alphabet
bool iterBool = false, recNoMemoBool = false, recMemoBool = false; // which type of dynamic programming to run
bool printBool = false; // whether to print table
bool readFileBool = false, genStringsBool = false; // whether to read in strings from file or generate strings randomly

// additions
int **table;
int total;
struct pair;
struct pair **pairsTable;
struct pair *helperArray;
int count;

typedef struct pair {
	int i;
	int j;
} Pair;

// functions follow

// determine whether a given string consists only of numerical digits
bool isNum(char s[]) {
	int i;
	bool isDigit=true;
	for (i=0; i<strlen(s); i++)
		isDigit &= s[i]>='0' && s[i]<='9';
	return isDigit;
}

// get arguments from command line and check for validity (return true if and only if arguments illegal)
bool getArgs(int argc, char *argv[]) {
	int i;
	alg_type = NONE;
	xLen = 0;
	yLen = 0;
	alphabetSize = 0;
	for (i = 1; i < argc; i++) // iterate over all arguments provided (argument 0 is name of this module)
		if (strcmp(argv[i],"-g")==0) { // generate strings randomly
			if (argc>=i+4 && isNum(argv[i+1]) && isNum(argv[i+2]) && isNum(argv[i+3])) { // must be three numerical arguments after this
				xLen=atoi(argv[i+1]); // get length of x
				yLen=atoi(argv[i+2]); // get length of y
				alphabetSize = atoi(argv[i+3]); // get alphabet size
				genStringsBool = true; // set flag to generate strings randomly
				i+=3; // ready for next argument
			}
			else
				return true; // must have been an error with -g arguments
		}
		else if (strcmp(argv[i],"-f")==0) { // read in strings from file
			if (argc>=i+2) { // must be one more argument (filename) after this)
				i++;
				filename = argv[i]; // get filename
				readFileBool = true; // set flag to read in strings from file
			}
			else
				return true; // must have been an error with -f argument
		}
		else if (strcmp(argv[i],"-i")==0) // iterative dynamic programming
			iterBool = true;
		else if (strcmp(argv[i],"-r")==0) // recursive dynamic programming without memoisation
			recNoMemoBool = true;
		else if (strcmp(argv[i],"-m")==0) // recursive dynamic programming with memoisation
			recMemoBool = true;
		else if (strcmp(argv[i],"-p")==0) // print dynamic programming table
			printBool = true;
		else if (strcmp(argv[i],"-t")==0) // which algorithm to run
			if (argc>=i+2) { // must be one more argument ("LCS" or "ED" or "SW")
				i++;
				if (strcmp(argv[i],"LCS")==0) { // Longest Common Subsequence
					alg_type = LCS;
					alg_desc = "Longest Common Subsequence\n";
					result_string = "Length of a longest common subsequence is";
				}
				else if (strcmp(argv[i],"ED")==0) { // Edit Distance
					alg_type = ED;
					alg_desc = "Edit Distance\n";
					result_string = "Edit distance is";
				}
				else if (strcmp(argv[i],"SW")==0) { // Smith-Waterman Algorithm
					alg_type = SW;
					alg_desc = "Smith-Waterman algorithm\n";
					result_string = "Length of a highest scoring local similarity is";
				}
				else
					return true; // none of these; illegal choice
			}
			else
				return true; // algorithm type not given
		else
			return true; // argument not recognised
		// check for legal combination of choices; return true (illegal) if user chooses:
		// - neither or both of generate strings and read strings from file
		// - generate strings with length 0 or alphabet size 0
		// - no algorithm to run
		// - no type of dynamic programming
		return !(readFileBool ^ genStringsBool) || (genStringsBool && (xLen <=0 || yLen <= 0 || alphabetSize <=0)) || alg_type==NONE || (!iterBool && !recMemoBool && !recNoMemoBool);
}

// read strings from file; return true if and only if file read successfully
bool readStrings() {
	// open file for read given by filename
	FILE * file;
	file = fopen(filename, "r");
	// firstly we will measure the lengths of x and y before we read them in to memory
	if (file) { // file opened successfully
		// first measure length of x
		bool done = false;
		int i;
		do { // read from file until newline encountered
			i = fgetc(file); // get next character
			if (i==EOF) { // EOF encountered too early (this is first string)
				// print error message, close file and return false
				printf("Incorrect file syntax\n");
				fclose(file);
				return false;
			}
			if ((char) i=='\n' || (char) i=='\r') // newline encountered
				done = true; // terminate loop
			else // one more character
				xLen++; // increment length of x
		} while (!done);
		// next measure length of y
		if ((char) i=='\r')
			fgetc(file); // get rid of newline character
		done = false;
		do { // read from file until newline or EOF encountered
			int i = fgetc(file); // get next character
			if (i==EOF || (char) i=='\n' || (char) i=='\r') // EOF or newline encountered
				done = true; // terminate loop
			else // one more character
				yLen++; // increment length of y
		} while (!done);
		fclose(file);
		// if either x or y is empty then print error message and return false
		if (xLen==0 || yLen==0) {
			printf("Incorrect file syntax\n");
			return false;
		}
		// now open file again for read
		file = fopen(filename, "r");
		// allocate memory for x and y
		x = malloc(xLen * sizeof(char));
		y = malloc(yLen * sizeof(char));
		// read in x character-by-character
		for (i=0; i<xLen; i++)
			x[i]=fgetc(file);
		i = fgetc(file); // read in newline between strings and discard
		if ((char) i=='\r')
			fgetc(file); // read \n character and discard if previous character was \r
		// read in y character-by-character
		for (i=0; i<yLen; i++)
			y[i]=fgetc(file);
		// close file and return boolean indicating success
		fclose(file);
		return true;
	}
	else { // notify user of I/O error and return false
		printf("Problem opening file %s\n",filename);
		return false;
	}
}

// generate two strings x and y (of lengths xLen and yLen respectively) uniformly at random over an alphabet of size alphabetSize
void generateStrings() {
	// allocate memory for x and y
	x = malloc(xLen * sizeof(char));
	y = malloc(yLen * sizeof(char));
	// instantiate the pseudo-random number generator (seeded based on current time)
	srand(time(NULL));
	int i;
	// generate x, of length xLen
	for (i = 0; i < xLen; i++)
		x[i] = rand()%alphabetSize +'A';
	// generate y, of length yLen
	for (i = 0; i < yLen; i++)
		y[i] = rand()%alphabetSize +'A';
}

// free memory occupied by strings
void freeMemory() {
	free(x);
	free(y);
}

/********************** HELPER FUNCTIONS *********************************/

// function to create an empty table
void createTable(int xLen, int yLen) {
	int row;
	table = (int **)malloc((xLen+1)*sizeof(int *)); // first column
	for (row = 0; row <= xLen; row++)
  	table[row] = (int *)malloc((yLen+1)*sizeof(int)); // rows
}

// function to initialise table with "0" values for first row and column
void initTable(int xLen, int yLen) {
	int i, j;
	for (i = 0; i <= xLen; i++)
		table[i][0] = 0;
	for (j = 1; j <= yLen; j++)
		table[0][j] = 0;
}

// function to initialise the whole table with "0"s
void initZeroTable(int xLen, int yLen) {
	int i, j;
	for (i = 0; i <= xLen; i++) {
		for (j = 0; j <= yLen; j++)
			table[i][j] = 0;
	}
}

// function to create table of pairs used for memoisation algorithms
void createPairsTable(int xLen, int yLen) {
	int row;
	pairsTable = (Pair **)malloc((xLen+1)*sizeof(Pair *)); // first column
	for (row = 0; row <= xLen; row++)
  	pairsTable[row] = (Pair *)malloc((yLen+1)*sizeof(Pair)); // rows
}

// function to create a helper array to check if table value was evaluated
void createHelperArray(int xLen, int yLen) {
	helperArray = malloc((xLen+1)*(yLen+1)*sizeof(Pair));
}

// function to check if an entry of the table has been evaluated or not
bool evaluated(int i, int j) {
 	int q = pairsTable[i][j].j;
 	if (q < 1 || q > count)
		return false;
	else
		return ((helperArray[q].i == i) && (helperArray[q].j == j));
}

// free memory used by table
void destroyTable(int xLen, int yLen) {
	int row;
	for (row = 0; row <= xLen; row++)
		free(table[row]);
	free(table);
}

// free memory used by the table of pairs
void destroyPairsTable(int xLen, int yLen) {
	int row;
	for (row = 0; row <= xLen; row++) {
		free(pairsTable[row]);
	}
	free(pairsTable);
}

// free memory allocated for the helper array
void destroyHelperArray() {
	free(helperArray);
}

// count number of digit an int has
int numDigits(int n) {
    if (n < 10) return 1;
    return 1 + numDigits(n/10);
}

// pretty print dynamic programming table
void printTable(int xLen, int yLen) {
	int i,j;
	// get width of first entry - usually largest entry
	int w = numDigits(table[0][0]) + 1;

	// first row
	printf ("%2s%2s%2s", " ", " ", " ");
	for (j = 0; j <= yLen; j++)
		printf("%*d", w, j);

	// second row
	printf ("\n%2s%2s%2s%*s", " ", " ", " ", w, " ");
	for (j = 0; j < yLen; j++)
		printf("%*c", w, y[j]);

	// third row
	printf ("\n%2s%2s%2s", " ", " ", " ");
	for (j = 0; j <= yLen; j++)
		for (i = 0; i < w; i++)
			printf("%1s", "_");

	// fourth row
	printf("\n%2s%2s%2s", "0", " ", "|");
	for (j = 0; j <= yLen; j++)
		printf("%*d", w, table[0][j]);
	printf("\n");

	// rest of rows
	for (i = 1; i <= xLen; i++) {
		printf("%2d%2c%2s", i, x[i-1], "|");
		for (j = 0; j <= yLen; j++)
			printf("%*d", w, table[i][j]);
		printf("\n");
	}
}

// pretty print dynamic programming table of pairs
void printPairsTable(int xLen, int yLen) {
	int i,j;

	// first row
	printf ("%2s%2s%2s", " ", " ", " ");
	for (j = 0; j <= yLen; j++)
		printf("%2d", j);

	// second row
	printf ("\n%2s%2s%2s%2s", " ", " ", " ", " ");
	for (j = 0; j < yLen; j++)
		printf("%2c", y[j]);

	// third row
	printf ("\n%2s%2s%2s", " ", " ", " ");
	for (j = 0; j <= yLen; j++)
		printf("__");

	// fourth row
	printf("\n%2s%2s%2s", "0", " ", "|");
	for (j = 0; j <= yLen; j++) {
		if (evaluated(0, j))
			printf("%2d", pairsTable[0][j].i);
		else
			printf("%2s", "-");
	}
	printf("\n");

	// rest of rows
	for (i = 1; i <= xLen; i++) {
		printf("%2d%2c%2s", i, x[i-1], "|");
		for (j = 0; j <= yLen; j++) {
			if (evaluated(i, j))
				printf("%2d", pairsTable[i][j].i);
			else
				printf("%2s", "-");
		}
		printf("\n");
	}
}

// function to print the optimal alignment for LCS
void ilcsAlign(int xLen, int yLen) {
	int index = table[xLen][yLen];
	char lcs[index+1];
	lcs[index] = '\0';

	int l = MAX(xLen, yLen) + (MAX(xLen, yLen) - index);
	char newX[l+1];
	char newY[l+1];
	char align[l+1];
	newX[l] = '\0';
	newY[l] = '\0';
	align[l] = '\0';

	int c;
	for (c = 0; c < l; c ++) {
		newX[c] = '-';
		newY[c] = '-';
		align[c] = ' ';
	}

	int i = xLen;
	int j = yLen;
	while (i > 0 && j > 0) {
		if (x[i-1] == y[j-1]) {
			lcs[index-1] = x[i-1];
			newX[l-1] = x[i-1];
			newY[l-1] = y[j-1];
			align[l-1] = '|';
			i--;
			j--;
			index--;
		} else if (table[i-1][j] > table[i][j-1]) {
			newX[l-1] = x[i-1];
			i--;
		}	else {
			newY[l-1] = y[j-1];
			j--;
		}
		l--;
	}

	if (i > 0 && j==0) {
		while (i > 0) {
			newX[l-1] = x[i-1];
			i--;
			l--;
		}
	}

	if (j > 0 && i==0) {
		while (j > 0) {
			newX[l-1] = y[j-1];
			j--;
			l--;
		}
	}

	printf("\nOptimal Alignment:\n");
	printf("%s\n", newX);
	printf("%s\n", align);
	printf("%s\n", newY);
}

// function to print the optimal alignment for LCS
void mlcsAlign(int xLen, int yLen) {
	int index = pairsTable[xLen][yLen].i;
	char lcs[index+1];
	lcs[index] = '\0';

	int l = MAX(xLen, yLen) + (MAX(xLen, yLen) - index);
	char newX[l + 1];
	char newY[l + 1];
	char align[l+1];
	newX[l] = '\0';
	newY[l] = '\0';
	align[l] = '\0';

	int c;
	for (c = 0; c < l; c ++) {
		newX[c] = '-';
		newY[c] = '-';
		align[c] = ' ';
	}

	int i = xLen;
	int j = yLen;
	while (i > 0 && j > 0 && l > 0) {
		if (x[i-1] == y[j-1]) {
			lcs[index-1] = x[i-1];
			newX[l-1] = x[i-1];
			newY[l-1] = y[j-1];
			align[l-1] = '|';
			i--;
			j--;
			index--;
		} else if (pairsTable[i-1][j].i > pairsTable[i][j-1].i) {
			newX[l-1] = x[i-1];
			i--;
		}	else {
			newY[l-1] = y[j-1];
			j--;
		}
		l--;
	}

	if (i > 0 && j==0) {
		while (i > 0) {
			newX[l-1] = x[i-1];
			i--;
			l--;
		}
	}

	if (j > 0 && i==0) {
		while (j > 0) {
			newX[l-1] = y[j-1];
			j--;
			l--;
		}
	}

	printf("\nOptimal Alignment:\n");
	printf("%s\n", newX);
	printf("%s\n", align);
	printf("%s\n", newY);
}

/*************** LONGEST COMMON SUBSEQUENCE ALGORITHM *********************/

// iterative LCS
int lcs(char *x, char *y) {
	int i, j = 0;

	// create and initialise table
	createTable(xLen, yLen);
	initTable(xLen, yLen);

	// calculate rest of values
	for (i = 1; i <= xLen; i++)
		for (j = 1; j <= yLen; j++)
			if (x[i-1] == y[j-1])
				table[i][j] = table[i-1][j-1] + 1;
			else
				table[i][j] = MAX(table[i-1][j], table[i][j-1]);

	// return the bottom-right value(length of largest common subsequence)
	return table[xLen][yLen];
}

// recursive version of LCS - helper
int rlcshelper(int i, int j) {
	total++;
	table[i][j]++;
	if ( i==0 || j==0)
		return 0;
	else if (x[i] == y[j])
		return 1 + rlcshelper(i-1, j-1);
	else
		return MAX(rlcshelper(i-1,j), rlcshelper(i, j-1));
}

// recursive algorithm
int rlcs(int m, int n) {
	total = 0;
	createTable(m, n);
	initZeroTable(m, n);
	rlcshelper(m, n);
	return total;
}

// recursive LCS with memoisation - helper
int mlcshelper(int i, int j) {
	if (!evaluated(i, j)) {
		count++;
		pairsTable[i][j].j = count;
		helperArray[count].i = i;
		helperArray[count].j = j;
		if (i == 0 || j == 0)
			pairsTable[i][j].i = 0;
		else if (x[i-1] == y[j-1])
			pairsTable[i][j].i = 1 + mlcshelper(i-1, j-1);
		else
			pairsTable[i][j].i = MAX(mlcshelper(i-1, j), mlcshelper(i, j-1));
	}
	return pairsTable[i][j].i;
}

// recursive LCS with memoisation
int mlcs(int m, int n) {
	count = 0;
	createPairsTable(m, n);
	createHelperArray(m, n);
	mlcshelper(m,n);
	return pairsTable[m][n].i;
}

/********************* EDIT DISTANCE ALGORITHM *****************************/

// iterative ED
int ed(char *x, char *y) {
	int i, j = 0;

	// create and initialise table
	createTable(xLen, yLen);

	// initialise first row and column of the table
	for (i = 0; i <= xLen; i++)
		table[i][0] = i;
	for (j = 1; j <= yLen; j++)
		table[0][j] = j;

	// calculate rest of edit distance
	for (i = 1; i <= xLen; i++)
		for (j = 1; j <= yLen; j++)
			if (x[i-1] == y[j-1])
				table[i][j] = table[i-1][j-1];
			else
				table[i][j] = MIN(table[i-1][j], MIN(table[i][j-1], table[i-1][j-1])) + 1;

	// return the bottom-right value(length of largest common subsequence)
	return table[xLen][yLen];
}

// recursive ED - helper
int redhelper(int i, int j) {
	total++;
	table[i][j]++;
	if (i==0 || j==0)
		return 0;
	else if (x[i] == y[j])
		return redhelper(i-1, j-1);
	else
		return MIN(redhelper(i-1,j), MIN(redhelper(i, j-1), redhelper(i-1, j-1))) + 1;
}

// recursive version of the ED alg
// -- prints how many times a table entry was computed
int red(int m, int n) {
	total = 0;
	createTable(m, n);
	initZeroTable(m, n);
	redhelper(m, n);
	return total;
}

// recursive ED with memoisation -- helper
int medhelper(int i, int j) {
	if (!evaluated(i, j)) {
		count++;
		pairsTable[i][j].j = count;
		helperArray[count].i = i;
		helperArray[count].j = j;
		if (i == 0)
		 	pairsTable[i][j].i =j;
		else if (j == 0)
			pairsTable[i][j].i = i;
		else if (x[i-1] == y[j-1])
			pairsTable[i][j].i = medhelper(i-1, j-1);
		else
			pairsTable[i][j].i = 1 + MIN(medhelper(i-1, j-1), MIN(medhelper(i-1, j), medhelper(i, j-1)));
	}
	return pairsTable[i][j].i;
}

// recursive ED with memoisation
int med(int m, int n) {
	count = 0;
	createPairsTable(m, n);
	createHelperArray(m, n);
	medhelper(m,n);
	return pairsTable[m][n].i;
}

/********************** SMITH-WATERMAN ALORITHM ****************************/
/** i.e. length of the highest scoring local similarity **/

// iterative version
int hsls(char *x, char *y) {
	int i, j, bestScore = 0;

	// create and initialise table
	createTable(xLen, yLen);
	initTable(xLen, yLen);

	// calculate rest of values, keeping track of bestScore
	for (i = 1; i <= xLen; i++)
		for (j = 1; j <= yLen; j++) {
			if (x[i-1] == y[j-1])
				table[i][j] = table[i-1][j-1] + 1;
			else
				table[i][j] = MAX(table[i-1][j] - 1, MAX(table[i][j-1] - 1, MAX(table[i-1][j-1] - 1, 0)));
			if (table[i][j] > bestScore)
				bestScore = table[i][j];
		}

	// return the bottom-right value(length of largest common subsequence)
	return bestScore;
}

// main method, entry point
int main(int argc, char *argv[]) {
	clock_t begin, end;
	double time_spent = 0.0;
	int result = 0;
	float calc = 0.0;
	bool isIllegal = getArgs(argc, argv); // parse arguments from command line
	if (isIllegal) // print error and quit if illegal arguments
		printf("Illegal arguments\n");
	else {
		printf("%s\n", alg_desc); // confirm algorithm to be executed
		bool success = true;
		if (genStringsBool)
			generateStrings(); // generate two random strings
		else
			success = readStrings(); // else read strings from file
		if (success) { // do not proceed if file input was problematic
			// confirm dynamic programming type
			// these print commamds are just placeholders for now
			if (iterBool) {
				printf("Iterative version\n");

				// start clock
				begin = clock();

				// choose alg
				if (alg_type==LCS)
					result = lcs(x,y);
				else if (alg_type==ED)
					result = ed(x,y);
				else if (alg_type==SW)
					result = hsls(x,y);

				// end clock
				end = clock();

				// print result
				printf("%s %d\n", result_string, result);

				// print dynamic programming table
				printf("Dynamic programming table:\n");
				printTable(xLen, yLen);

				if (alg_type==LCS)
					ilcsAlign(xLen, yLen);

				// destroy table
				destroyTable(xLen, yLen);

				// print time
				time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("\nTime taken: %0.2f seconds\n", time_spent);
			}
			if (recMemoBool && (alg_type==LCS || alg_type==ED)) {
				printf("Recursive version with memoisation\n");

				// start clock
				begin = clock();

				// choose alg
				if (alg_type==LCS)
					result = mlcs(xLen, yLen);
				else if (alg_type==ED)
					result = med(xLen, yLen);

				// end clock
				end = clock();

				if (printBool) {
					// print dynamic programming table
					printf("Dynamic programming table:\n");
					printPairsTable(xLen, yLen);
				}

				// print result
				printf("%s %d\n", result_string, result);

				if (alg_type==LCS)
					mlcsAlign(xLen, yLen);


				// print num of entries computed
				printf("\nNumber of table entries computed: %d\n", count);

				// print proportion details
				int tsize = (xLen+1)*(yLen+1);
				calc = ((float)count/(float)tsize)*100.0;
				printf("Proportion of table computed: %.1f%%\n", calc);

				// destroy table
				destroyPairsTable(xLen, yLen);
				// destroy helper array
				destroyHelperArray();

				// print time
				time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("Time taken: %0.2f seconds\n", time_spent);
			}
			if (recNoMemoBool && (alg_type==LCS || alg_type==ED)) {
				printf("Recursive version without memoisation\n");

				// start clock
				begin = clock();

				// choose alg
				if (alg_type==LCS)
					result = rlcs(xLen, yLen);
				else if (alg_type==ED)
					result = red(xLen, yLen);

				// end clock
				end = clock();

				if (printBool) {
					// print dynamic programming table
					printf("Dynamic programming table:\n");
					printTable(xLen, yLen);
				}

				// destroy table
				destroyTable(xLen, yLen);

				// print result
				printf("\nTotal number of times a table entry computed: %d\n", result);

				// print time
				time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("Time taken: %0.2f seconds\n", time_spent);
			}
			freeMemory(); // free memory occupied by strings
		}
	}
	return 0;
}
