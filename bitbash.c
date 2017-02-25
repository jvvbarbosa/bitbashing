#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct _data_block {
  char *data;
  unsigned long len;
}DATA_BLOCK;


int *recombulator(DATA_BLOCK *raw_data, int start_ptr, int end_ptr, int word_size){
  int *decod_data;
  int raw_lenght;
  int nwords;
  unsigned int d_bit = 0, r_bit = 0;
  unsigned int byte_idx = start_ptr;
  unsigned int word_idx = 0;
  unsigned int mask;
  unsigned int place_holder;

  raw_lenght = end_ptr-start_ptr;

  /*number of words that can be decoded
  this formula makes ceil() without the call to math.lib*/
  nwords = 1 + (((raw_lenght * 8) - 1) / word_size);

  /*make sure at least one word can be constructed*/
  if (raw_lenght == 0 | raw_lenght < nwords) {
    perror("not enough bytes to be decoded");
    return NULL;
  }


  decod_data = (int *)calloc(nwords, sizeof(int));
  if (decod_data == NULL) {
    perror("Could not allocate memory for decod_data");
    return NULL;
  }


  while (word_idx < nwords && byte_idx <= end_ptr) {
    while (d_bit < word_size) {
        /*one bit mask to extract 1 bit from raw_data byte*/
        mask = 1 << r_bit;

        /*in case of having to do 1 bit shifts in raw data, we need
        a mechanism able to place a bit anywhere inside a decoded word.
        For instance in the begining of the process if a bit offset happens
        r_bit will be ahead of d_bit. In this case we need to place the extracted
        bit in a lower position inside the decoded word*/
        if (d_bit >= r_bit) {
          place_holder = mask << (d_bit - r_bit);
        }else{
          place_holder = mask >> (r_bit - d_bit);
        }
        /* << (d_bit - r_bit) allows to control bits placed above 8th position*/
        decod_data[word_idx] = ((raw_data->data[byte_idx] & mask) << (d_bit - r_bit)) | decod_data[word_idx];
        r_bit++;
        d_bit++;

        /*reset bit index for raw bytes and move to next byte*/
        if (d_bit == 8) {
          byte_idx++;
          r_bit = 0;
        }
    }
    /*reset bit index for decoded word and move to next word*/
    word_idx++;
    d_bit = 0;
    r_bit = 0;
  }

  return decod_data;

}



DATA_BLOCK *load_data(const char* filename){
  FILE *f;
  long end_ptr, start_ptr;
  unsigned long data_size;
  DATA_BLOCK *raw_data;
  int read_size;

  f = fopen(filename, "r");

  if (f == NULL) {
    perror("Could not open file: ");
    return NULL;
  }

  /*find number of bytes in input file*/
  start_ptr = ftell(f);
	fseek(f, 0L, SEEK_END);
	end_ptr = ftell(f);
	if(end_ptr == start_ptr){
		perror("Input file has zero elements\n");
		return NULL;
	}
	fseek(f, start_ptr, SEEK_SET);
	data_size = end_ptr - start_ptr;

  /*allocate memory to store raw data*/
  raw_data = (DATA_BLOCK *) malloc(sizeof(DATA_BLOCK));
  if (raw_data == NULL) {
    perror("Could not allocate memory for DATA_BLOCK");
  }
  raw_data->len = data_size;

  raw_data->data = (char*)malloc(data_size * sizeof(char));
  if (raw_data->data == NULL) {
    perror("Could not allocate memory for raw_data");
    return NULL;
  }

  read_size = fread(raw_data->data, 1, data_size, f);

	if(read_size != data_size){
		if(feof(f)){
			perror("Unexpected end of file in input file");
		}else if( ferror(f) ){
			perror("Error: in fread() function");
		}
		free(raw_data->data);
  }

  return raw_data;
}






int main(int argc, char const *argv[]) {
  FILE *output;
  DATA_BLOCK *raw_data;
  int *decoded;
  int i;

  if (argc < 2) {
    perror("Please provide input file path");
    exit(-1);
  }

  raw_data = load_data(argv[1]);

  decoded = recombulator(raw_data, 0, raw_data->len, 12);

  output = fopen("output.csv", "w");



  for (i = 0; i < 1000; i++) {
    printf("%x;", decoded[i]);
    //printf(output, "%x;",(unsigned int)decoded[i]);
  }
  printf("\n");


  return 0;
}
