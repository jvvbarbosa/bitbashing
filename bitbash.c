#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
typedef struct _decode_block{
	unsigned int word_size;// in bits
	unsigned int data_size;// in bytes
	unsigned int nbits;
	unsigned int nwords;
	unsigned long *words;
	char *bits;
	char *raw_data;
} DECODE_BLOCK;


void _insert_bit_at(char * buffer, int bit, int pos){

	int byteIndex = pos/8;
	int bitIndex = pos%8;
	char mask = 1 << bitIndex;
	buffer[byteIndex] = (buffer[byteIndex] & ~mask ) | ((bit << bitIndex) & mask);
} 

int build_words(DECODE_BLOCK *block, unsigned int start_word, unsigned int nwords){

	unsigned int start_bit_index = start_word*block->word_size;
	unsigned int byte_idx = start_bit_index >> 3u; 
	unsigned int bit_idx = start_bit_index;
	unsigned int word = 0;
	unsigned int word_bit = 0;
	unsigned int shift;
	unsigned long mask;


	while(word < nwords){
		while(word_bit < block->word_size && word < nwords){
			byte_idx = bit_idx >> 3u; 
			shift = bit_idx & 7u; 
			mask = 1 << word_bit;
			block->words[word] = (block->words[word] & ~mask) | (((block->raw_data[byte_idx] >> shift) << word_bit) & mask);
			bit_idx++;
			word_bit++;
		}
		word_bit = 0;
		word++;
	}   

	return 0;

}

int build_bit_char_array(DECODE_BLOCK *block, unsigned int start_word, unsigned int nwords){

	unsigned int start_bit_index = start_word*block->word_size;
	unsigned int byte_idx = start_bit_index >> 3u; 
	unsigned int bit_idx = start_bit_index;
	unsigned int bit = 0;
	unsigned word = 0;
	unsigned int shift;

	for(word = 0; word < nwords; ++word){
		byte_idx = bit_idx >> 3u;
		shift = bit_idx & 7u;
		block->bits[bit++] = (block->raw_data[byte_idx] >> shift) & 0x1;
		bit_idx++;
	}
	return 0;
} 

void destroy_decode_block(DECODE_BLOCK *block){
	if(block != NULL){
		if(block->bits != NULL)
			free(block->bits);
		if(block->words != NULL)
			free(block->words);
		if(block->raw_data != NULL)
			free(block->raw_data);
		free(block);
	}
}

DECODE_BLOCK *_create_decode_block(unsigned int word_size, unsigned long data_size){
	if(word_size > sizeof(long)*8){
		fprintf(stderr, "This module only supports word_sizes up to 64 bits. Request word size of %d\n", word_size);
		return NULL;
	}
	unsigned long words; 
	words = ((long)data_size*8)/word_size;
	DECODE_BLOCK * block = (DECODE_BLOCK *) malloc(sizeof(DECODE_BLOCK));
	if(block == NULL){
		perror("Could not allocate DECODE_BLOCK.\n");
		goto error;
	}
	block->raw_data = (char*) malloc(data_size*sizeof(char));
	if(block->raw_data == NULL){
		perror("Could not allocate DECODE_BLOCK.\n");
		goto error;
	}
	memset(block->raw_data, 0, data_size*sizeof(char));
	block->words = (unsigned long*) malloc(words);
	if(block->words == NULL){
		perror("Could not allocate DECODE_BLOCK.\n");
		goto error;
	}
	memset(block->words, 0, words);
	block->bits = (char *) malloc(data_size*8);
	if(block->bits == NULL){
		perror("Could not allocate DECODE_BLOCK.\n");
		goto error;
	}

	memset(block->bits, 0, data_size*8);
	return block;
error:
	
	destroy_decode_block(block);
	return NULL;

}
DECODE_BLOCK * create_decode_block(char *filename, unsigned int word_size){
	FILE *f; 
	long end_ptr;
	long start_ptr;
	unsigned long data_size;

	if(word_size = 0)
		return NULL;

	f = fopen(filename, "r");

	if( f == NULL){
		perror("Could not open file: ");
		return NULL;
	}

	start_ptr = ftell(f);
	fseek(f, 0L, SEEK_END);
	end_ptr = ftell(f);
	if(end_ptr == start_ptr){
		fprintf(stderr, "File %s has zero elements\n", filename);
		return NULL;
	}
	fseek(f, start_ptr, SEEK_SET);
	data_size = end_ptr - start_ptr;
	DECODE_BLOCK * block = _create_decode_block(word_size, data_size);

	if(block == NULL){
		perror("Could not allocate memory\n");
		goto error;
	}

	block->word_size = word_size;
	block->data_size = data_size;
	block->nbits = data_size*8;
	block->nwords = block->nbits/word_size;
	int read_size = fread(block->raw_data, 1, data_size, f);
	if(read_size != data_size){
		if(feof(f)){
			fprintf(stderr, "Unexpected end of file @ %s\n", __func__);
		}else if( ferror(f) ){
			fprintf(stderr, "Error: %s\n. file %s, function %s",
					strerror(errno), __FILE__, __func__);
		}	
		goto error;	
	}

	return block;
error:
	destroy_decode_block(block);
	return NULL;
}

 

void myprintHelp(){
	printf("Usage:\n");
	printf("$bitbash --file <filename> [options]\n\n");
	printf("---- Options ---\n");
	printf("--word-size <decimal value> (default value is 12\n\n");
	printf("--start_word <decimal value> (default value is 0. Sets the word the reformating should start at.)\n\n");
	printf("--words <decimal value> (Number of words that should be reformated. Will start from start-word. Reformats the whole file if not specified.)\n\n");
	printf("--file <string> (specify file name, must be specified\n");
	printf("--save-file <string> (if not specified, output will be saved in output.csv");

}

int write_output(DECODE_BLOCK * block, char *filename, int nwords){
	
	FILE *f;

	if(nwords > block->nwords)
		return -1;
	int nbits;
	nbits = nwords*block->word_size;
	f = fopen(filename, "w");

	if( f == NULL){
		perror("Could not open file: ");
		return -1;
	}
	
	
	int i;
	for(i = 0; i < nbits - 1; ++i){
		fputc(block->bits[i], f);
		fputc(',', f);
	}	
	fputc(block->bits[nbits-1], f);
}

int main(int argc, char *argv[]){

	unsigned int word_size = 12; //default value	
	unsigned int start_word= 0;
	unsigned int words = 0;
	int fileFlag = 0;
	int savefileFlag = 0;
	char filename[1024];
	char savefile[1024];
	char opt;
	int optionIndex;

	static const struct option long_options[] = {
		{"word-size", required_argument, NULL, 1},
		{"start-word", required_argument, NULL, 2},
		{"words", required_argument, NULL, 3},
		{"file", required_argument, NULL, 4},
		{"save-file", required_argument, NULL, 5},
		{0,0,0,0}
	};

	while((opt = getopt_long(argc, argv, "",long_options, &optionIndex)) != -1){
		switch(opt){
			case 1:
				if(sscanf(optarg, "%u", &word_size) == 0){
					fprintf(stderr, "Wrong format for word-size\n");
					exit(EXIT_FAILURE);
				}else{
					if(word_size == 0){
						fprintf(stderr, "Word size cannot be 0.\n");
						exit(EXIT_FAILURE);
					}
				}
				break;
			case 2:
				if(sscanf(optarg, "%u", &start_word) == 0){
					fprintf(stderr, "Wrong format for start-word\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 3:
				if(sscanf(optarg, "%u", &words) == 0){
					fprintf(stderr, "Wrong format for <words>\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 4:
				if(sscanf(optarg, "%s", filename) == 0){
					fprintf(stderr, "Could not read file name.\n");
					exit(EXIT_FAILURE);
				} 
				fileFlag = 1;
				break;
			case 5:
				if(sscanf(optarg, "%s", savefile) == 0){
					fprintf(stderr, "Could not read save-file name.\n");
					exit(EXIT_FAILURE);
				} 
				savefileFlag = 1;
				break;
			default:
				myprintHelp();
				break;
		
		}
	
	}
	

	if(!fileFlag){
		fprintf(stderr, "No file name defined, use option --file to define file name.\n");
	}
	DECODE_BLOCK * block = create_decode_block(filename, word_size);
	if(block == NULL)
		exit(-1);
	if(words == 0 && block->word_size != 0){
		words = block->data_size*8/block->word_size;
	}

	
	int err = build_words(block, start_word, words); 
	if(err){
		fprintf(stderr, "Error getting data in %d words.\n", word_size);
	}

	err = build_bit_char_array(block, start_word, words);

	if(err){
		fprintf(stderr, "Error getting data in bits (represented by chars).\n");
	}

	if(savefileFlag){
		err = write_output(block, savefile, block->nwords);	
	}else{
		err = write_output(block, "output.csv", block->nwords);	
	}


	exit(0);

}



