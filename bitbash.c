#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int SYNC_WORDS[4] = {0X247,0x5B8,0xA47,0xDB8};
int W_SEC = 512;
int W_SIZE = 12;


typedef struct _data_block {
  char *data;
  unsigned long len;
}DATA_BLOCK;

typedef struct _syncro_list {
  int word_sec;
  int *sync_words;
  int shift;
  unsigned long *first;
  unsigned long *second;
  unsigned long *third;
  unsigned long *fourth;
}SYNCRO_LIST;


SYNCRO_LIST *create_syncro_list(int *sync_words, int word_sec, int nwords){
  SYNCRO_LIST *sync_list;
  int nsync;

  sync_list = (SYNCRO_LIST *)malloc(sizeof(SYNCRO_LIST));
  if (sync_list == NULL) {
    perror("Could not allocate memory for sync_list");
    return NULL;
  }

  sync_list->word_sec = word_sec;
  sync_list->shift = 0;
  sync_list->sync_words = sync_words;

  nsync = (nwords/4)/word_sec;

  sync_list->first = (unsigned long *)malloc(nsync*sizeof(unsigned long));
  if (sync_list->first == NULL) {
    perror("Could not allocate memory for #1 sync word array");
    return NULL;
  }

  sync_list->second = (unsigned long *)malloc(nsync*sizeof(unsigned long));
  if (sync_list->second == NULL) {
    perror("Could not allocate memory for #2 sync word array");
    return NULL;
  }

  sync_list->third = (unsigned long *)malloc(nsync*sizeof(unsigned long));
  if (sync_list->third == NULL) {
    perror("Could not allocate memory for #3 sync word array");
    return NULL;
  }

  sync_list->fourth = (unsigned long *)malloc(nsync*sizeof(unsigned long));
  if (sync_list->fourth == NULL) {
    perror("Could not allocate memory for #4 sync word array");
    return NULL;
  }

  return sync_list;

}



int find_syncro(DATA_BLOCK *raw_data, SYNCRO_LIST *sync_list,
                        unsigned long start_ptr, unsigned long end_ptr,
                        int word_size){

  unsigned int d_bit = 0;
  unsigned int r_bit;
  unsigned int byte_idx = start_ptr;
  unsigned int word_idx = 0;
  unsigned int mask;
  unsigned int place_holder;
  int temp=0;
  int j = 0;
  int sync_pos[4] = {0,0,0,0};
  int shift = 0;

  for (shift = 0; shift < 4; shift++) {

    r_bit = shift;

    while (byte_idx <= end_ptr) {
      while (d_bit < word_size) {

          mask = 1 << r_bit;

          if (d_bit >= r_bit) {
            place_holder = mask << (d_bit - r_bit);
          }else{
            place_holder = mask >> (r_bit - d_bit);
          }

          temp = temp | place_holder;

          r_bit++;
          d_bit++;

          if (r_bit == 8) {
            byte_idx++;
            r_bit = 0;
          }
      }

      if (temp == sync_list->sync_words[1]) {
        if (byte_idx - sync_pos[4] == sync_list->word_sec){
          return shift;
        }else{
          sync_pos[1] = byte_idx;
        }
      }else if (temp == sync_list->sync_words[2]){
        if (byte_idx - sync_pos[1] == sync_list->word_sec){
          return shift;
        }else{
          sync_pos[2] = byte_idx;
        }
      }else if (temp == sync_list->sync_words[3]){
        if (byte_idx - sync_pos[2] == sync_list->word_sec){
          return shift;
        }else{
          sync_pos[3] = byte_idx;
        }
      }else if (temp == sync_list->sync_words[4]){
        if (byte_idx - sync_pos[3] == sync_list->word_sec){
          return shift;
        }else{
          sync_pos[4] = byte_idx;
        }
      }


      temp = 0;
      j++;

      d_bit = 0;
      r_bit = 0;

    }
  }

  return -1;

}


int *recombulator(DATA_BLOCK *raw_data, int start_ptr, int end_ptr, int word_size, int bit_shift){
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
          place_holder = (raw_data->data[byte_idx] & mask) << (d_bit - r_bit);
        }else{
          place_holder = (raw_data->data[byte_idx] & mask) >> (r_bit - d_bit);
        }

        printf("mask=%d place=%d\n", mask, place_holder);
        /* << (d_bit - r_bit) allows to control bits placed above 8th position*/
        decod_data[word_idx] = place_holder | decod_data[word_idx];
        r_bit++;
        d_bit++;

        /*reset bit index for raw bytes and move to next byte*/
        if (r_bit == 8) {
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

  // decoded = recombulator(raw_data, 0, 10, 12);
  //
  // output = fopen("output.csv", "w");
  //
  // for (i = 0; i < 1000; i++) {
  //   printf("%x;", decoded[i]);
  //   //printf(output, "%x;",(unsigned int)decoded[i]);
  // }
  // printf("\n");


  return 0;
}
