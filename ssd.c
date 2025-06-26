//ID - 202303005  Name - Patel Hemil Ashishkumar
//ID - 202303051  Name - Rom Tala

//compile: gcc ssd.c -o ssd
//commands:
//ssd -T ideal
//ssd -T ideal -L w10:a -l 30 -B 3 -p 10
//ssd -T ideal -L w10:a,r10,t10 -l 30 -B 3 -p 10
//ssd -T ideal -L w10:a,r10,t10 -l 30 -B 3 -p 10 -C
//ssd -T ideal -L w10:a,r10,t10 -l 30 -B 3 -p 10 -F
//ssd -T ideal -l 30 -B 3 -p 10 -n 5 -s 10
//ssd -T ideal -l 30 -B 3 -p 10 -n 5 -s 10 -q
//ssd -T direct -l 30 -B 3 -p 10 -n 5 -s 10 -C 
//ssd -T direct -l 30 -B 3 -p 10 -C -L w9:f,t9,r9
//ssd -T log -l 30 -B 3 -p 10 -s 10 -n 5 -C
//ssd -T log -l 30 -B 3 -p 10 -s 10 -n 5 -S
//ssd -T log -l 30 -B 3 -p 10 -s 10 -n 60 -G 3 -g 2 -C -F -J


//---------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
// Define constants for SSD types
#define TYPE_DIRECT 1
#define TYPE_LOGGING 2
#define TYPE_IDEAL 3

// Define constants for page states
#define STATE_INVALID 1
#define STATE_ERASED 2
#define STATE_VALID 3

typedef struct {
    // Type
    int ssd_type;

    // Size
    int num_logical_pages;
    int num_blocks;
    int pages_per_block;
    int num_pages;//by us
    // Parameters
    int block_erase_time;
    int page_program_time;
    int page_read_time;

    // Page states and data
    int *state;
    char **data;

    // LOGGING-related attributes
    int current_page;
    int current_block;

    // GC-related attributes
    int gc_count;
    int gc_current_block;
    int gc_high_water_mark;
    int gc_low_water_mark;

    // Used for GC trace and state display
    int gc_trace;
    int show_state;

    // GC usage tracking
    int *gc_used_blocks;

    // Live counts for GC
    int *live_count;

    // FTL maps
    int *forward_map;
    int *reverse_map;

    // Stats
    int *physical_erase_count;
    int *physical_read_count;
    int *physical_write_count;

    int physical_erase_sum;
    int physical_write_sum;
    int physical_read_sum;

    int logical_trim_sum;
    int logical_write_sum;
    int logical_read_sum;

    int logical_trim_fail_sum;
    int logical_write_fail_sum;
    int logical_read_fail_sum;
} SSD;

// Function to initialize the SSD
SSD *ssd_init(const char *ssd_type, int num_logical_pages, int num_blocks,
              int pages_per_block, int block_erase_time, int page_program_time,
              int page_read_time, int high_water_mark, int low_water_mark,
              int trace_gc, int show_state) {
    // Allocate memory for the SSD
    SSD *ssd = (SSD *)malloc(sizeof(SSD));

    // Set type
    if (strcmp(ssd_type, "direct") == 0) {
        ssd->ssd_type = TYPE_DIRECT;
    } else if (strcmp(ssd_type, "log") == 0) {
        ssd->ssd_type = TYPE_LOGGING;
    } else if (strcmp(ssd_type, "ideal") == 0) {
        ssd->ssd_type = TYPE_IDEAL;
    } else {
        printf("Bad SSD type: %s\n", ssd_type);
        free(ssd);
        exit(1);
    }

    // Set sizes
    ssd->num_logical_pages = num_logical_pages;
    ssd->num_blocks = num_blocks;
    ssd->pages_per_block = pages_per_block;

    // Set parameters
    ssd->block_erase_time = block_erase_time;
    ssd->page_program_time = page_program_time;
    ssd->page_read_time = page_read_time;

    // Allocate and initialize state and data
    ssd->num_pages = num_blocks * pages_per_block;
    ssd->state = (int *)malloc(ssd->num_pages * sizeof(int));
    ssd->data = (char **)malloc(ssd->num_pages * sizeof(char *));
    for (int i = 0; i < ssd->num_pages; i++) {
        ssd->state[i] = STATE_INVALID;
        ssd->data[i] = (char *)malloc(sizeof(char)); // Initialize as empty
        ssd->data[i][0] = '\0';
    }

    // Initialize LOGGING attributes
    ssd->current_page = -1;
    ssd->current_block = 0;

    // Initialize GC attributes
    ssd->gc_count = 0;
    ssd->gc_current_block = 0;
    ssd->gc_high_water_mark = high_water_mark;
    ssd->gc_low_water_mark = low_water_mark;
    ssd->gc_trace = trace_gc;
    ssd->show_state = show_state;

    ssd->gc_used_blocks = (int *)calloc(num_blocks, sizeof(int));
    ssd->live_count = (int *)calloc(num_blocks, sizeof(int));

    // Initialize FTL maps
    ssd->forward_map = (int *)malloc(num_logical_pages * sizeof(int));
    ssd->reverse_map = (int *)malloc(ssd->num_pages * sizeof(int));
    for (int i = 0; i < num_logical_pages; i++) {
        ssd->forward_map[i] = -1;
    }
    for (int i = 0; i < ssd->num_pages; i++) {
        ssd->reverse_map[i] = -1;
    }

    // Initialize stats
    ssd->physical_erase_count = (int *)calloc(num_blocks, sizeof(int));
    ssd->physical_read_count = (int *)calloc(num_blocks, sizeof(int));
    ssd->physical_write_count = (int *)calloc(num_blocks, sizeof(int));
    for(int i=0;i<num_blocks;i++){
        ssd->physical_erase_count[i]=0;
        ssd->physical_read_count[i]=0;
        ssd->physical_write_count[i]=0;
    }

    ssd->physical_erase_sum = 0;
    ssd->physical_write_sum = 0;
    ssd->physical_read_sum = 0;

    ssd->logical_trim_sum = 0;
    ssd->logical_write_sum = 0;
    ssd->logical_read_sum = 0;

    ssd->logical_trim_fail_sum = 0;
    ssd->logical_write_fail_sum = 0;
    ssd->logical_read_fail_sum = 0;

    return ssd;
}
//=========================================================================================
//blocks in use---------------------------------------------------------------
int blocks_in_use(SSD *ssd) {
    int used = 0;
    for (int i = 0; i < ssd->num_blocks; i++) {
        used += ssd->gc_used_blocks[i];
    }
    return used;
}

//physical erase--------------------------------------------------------
void physical_erase(SSD *ssd, int block_address) {
    int page_begin = block_address * ssd->pages_per_block;
    int page_end = page_begin + ssd->pages_per_block - 1;

    for (int page = page_begin; page <= page_end; page++) {
        ssd->data[page] = "";  // Clear the data (empty string)
        ssd->state[page] = STATE_ERASED;  // Set state to erased
    }

    // Mark block as not in use
    ssd->gc_used_blocks[block_address] = 0;

    // Update stats
    ssd->physical_erase_count[block_address]++;
    ssd->physical_erase_sum++;
    return;
}
//physical program---------------------------------------------------------------
void physical_program(SSD *ssd, int page_address, const char *data) {
  //physical_program(ssd, page_address, data);

    // Free any previously allocated memory to avoid memory leaks
    if (ssd->data[page_address] != NULL) {
        free(ssd->data[page_address]);
    }

    // Allocate memory for the new data, including space for the null terminator
    ssd->data[page_address] = (char *)malloc((strlen(data) + 1) * sizeof(char));
    if (ssd->data[page_address] == NULL) {
        perror("Memory allocation failed");
        return;
    }

    strncpy(ssd->data[page_address], data, strlen(data) + 1); // Copy data
    //printf("Copy Done!\n");
    //ssd->data[page_address]=data;
    ssd->state[page_address] = STATE_VALID;  // Set state to valid
    //printf("Set State Done!\n");
    // Update stats
    int block_index = page_address / ssd->pages_per_block;
    ssd->physical_write_count[block_index]++;
    ssd->physical_write_sum++;
}

//physical read-----------------------------------------------------------------------
char *physical_read(SSD *ssd, int page_address) {
    // Update stats
    ssd->physical_read_count[page_address / ssd->pages_per_block]++;
    ssd->physical_read_sum++;

    // Return the data at the specified page address
    return ssd->data[page_address];
}

// read direct------------------------------------------------------------------------
char *read_direct(SSD *ssd,int address){
    return physical_read(ssd,address);
}

//write direct-----------------------------------------------------------------------------------
char *write_direct(SSD *ssd, int page_address, const char *data) {
    int block_address = page_address / ssd->pages_per_block;
    int page_begin = block_address * ssd->pages_per_block;
    int page_end = page_begin + ssd->pages_per_block - 1;

    // Temporary storage for valid data in the block
    typedef struct {
        int page;
        char *data;
    } PageData;
    //printf("Struct Done\n");
    PageData *old_data_list = (PageData *)malloc((page_end - page_begin + 1) * sizeof(PageData));
    int old_count = 0;

    // Collect valid pages
    for (int old_page = page_begin; old_page <= page_end; old_page++) {
        if (ssd->state[old_page] == STATE_VALID) {
            old_data_list[old_count].page = old_page;
            old_data_list[old_count].data = strdup(ssd->data[old_page]); // Copy data
            old_count++;
        }
    }
    //printf("Collection of valid pages done\n");
    // Erase the block
    physical_erase(ssd, block_address);
    //printf("Physical Erease Done\n");
    // Reprogram the valid pages
    for (int i = 0; i < old_count; i++) {
        if (old_data_list[i].page != page_address) {
            physical_program(ssd, old_data_list[i].page, old_data_list[i].data);
        }
        free(old_data_list[i].data); // Free the temporary copy
    }
    //printf("Reprogrming valid pages done\n");
    free(old_data_list);

    // Program the new data
    //printf("%d\t%s\n",page_address,data);
    physical_program(ssd, page_address, data);
    //printf("Programming new data done\n");
    // Update mappings
    ssd->forward_map[page_address] = page_address;
    ssd->reverse_map[page_address] = page_address;
    //printf("Mappings update done\n");
    return "success";
}


//write Ideal ----------------------------------------------------------------------------
char *write_ideal(SSD *ssd, int page_address, const char *data) {
    // Program the page directly
    physical_program(ssd, page_address, data);

    // Update mappings
    ssd->forward_map[page_address] = page_address;
    ssd->reverse_map[page_address] = page_address;

    return "success";
}

//Is block free-------------------------------------------------------------------------------------------
int is_block_free(SSD *ssd, int block) {
    int first_page = block * ssd->pages_per_block;

    // Check if the first page of the block is in an invalid or erased state
    if (ssd->state[first_page] == STATE_INVALID || ssd->state[first_page] == STATE_ERASED) {
        if (ssd->state[first_page] == STATE_INVALID) {
            // Perform a physical erase on the block
            physical_erase(ssd, block);
        }

        // Update the current block and page
        ssd->current_block = block;
        ssd->current_page = first_page;
        ssd->gc_used_blocks[block] = 1;

        return 1; // Block is free
    }

    return 0; // Block is not free
}

//get cursor---------------------------------------------------------------------------
int get_cursor(SSD *ssd) {
    // If no valid current page, search for a free block
    if (ssd->current_page == -1) {
        for (int block = ssd->current_block; block < ssd->num_blocks; block++) {
            if (is_block_free(ssd, block)) {
                return 0;
            }
        }

        for (int block = 0; block < ssd->current_block; block++) {
            if (is_block_free(ssd, block)) {
                return 0;
            }
        }

        return -1; // No free blocks found
    }

    return 0; // Cursor is valid
}
//update cursor---------------------------------------------------------------------
void update_cursor(SSD *ssd) {
    ssd->current_page++;

    // Check if the page pointer has moved beyond the block boundary
    if (ssd->current_page % ssd->pages_per_block == 0) {
        ssd->current_page = -1;
    }
}
//write logging---------------------------------------------------------------------------
 char* write_logging(SSD *ssd, int page_address, const char *data) {
    
    // Ensure there's space for writing
    if (get_cursor(ssd) == -1) {
        ssd->logical_write_fail_sum++;
        return "failure: device full";
    }

    // Assert that the current page is in an erased state
    assert(ssd->state[ssd->current_page] == STATE_ERASED);

    // Perform a physical program operation
    physical_program(ssd, ssd->current_page, data);

    // Update the FTL maps
    ssd->forward_map[page_address] = ssd->current_page;
    ssd->reverse_map[ssd->current_page] = page_address;

    // Move the cursor forward
    update_cursor(ssd);

    return "success";
}


//printable state--------------------------------------------------------------
const char *printable_state(int state) {
    if (state == STATE_INVALID) {
        return "i";
    } else if (state == STATE_ERASED) {
        return "E";
    } else if (state == STATE_VALID) {
        return "v";
    } else {
        printf("bad state %d\n", state);
        exit(1);
    }
}

//dump-------------------------------------------------------------------------
void dump(SSD *ssd) {
    // FTL
    printf("FTL   ");
    int count = 0;
    int ftl_columns = (ssd->pages_per_block * ssd->num_blocks) / 7;
    for (int i = 0; i < ssd->num_logical_pages; i++) {
        if (ssd->forward_map[i] == -1) continue;
        count++;
        printf("%3d:%3d ", i, ssd->forward_map[i]);
        if (count > 0 && count % ftl_columns == 0) {
            printf("\n      ");
        }
    }
    if (count == 0) {
        printf("(empty)");
    }
    printf("\n");

    // Flash layout: Blocks
    printf("Block ");
    for (int i = 0; i < ssd->num_blocks; i++) {
        char out_str[16];
        sprintf(out_str, "%d", i);
        printf("%s%*s", out_str, ssd->pages_per_block - (int)strlen(out_str) + 1, "");
    }
    printf("\n");

    // Pages
    // Calculate the maximum length of the page numbers
    int max_len = 0, temp = ssd->num_pages - 1;
    while (temp > 0) {
        temp /= 10;
        max_len++;
    }

    // Loop for the vertical lines
    for (int n = max_len; n > 0; n--) {
        if (n == max_len) {
            printf("Page  ");
        } else {
            printf("      ");
        }

        // Loop through each page number
        for (int i = 0; i < ssd->num_pages; i++) {
            char out_str[10];  // Buffer to store the string representation of the number
            snprintf(out_str, sizeof(out_str), "%0*d", max_len, i); // Zero-pad to max_len

            // Print the character at position `max_len - n`
            printf("%c", out_str[max_len - n]);

            // Add a space after every 10 numbers for better readability
            if ((i + 1) % 10 == 0) {
                printf(" ");
            }
        }
        printf("\n");  // Move to the next line
    }

    // States
    printf("State ");
    for (int i = 0; i < ssd->num_pages; i++) {
        printf("%s", printable_state(ssd->state[i]));
        if (i > 0 && (i + 1) % 10 == 0) {
            printf(" ");
        }
    }
    printf("\n");

    // Data
    printf("Data  ");
    for (int i = 0; i < ssd->num_pages; i++) {
        if (ssd->state[i] == STATE_VALID) {
            printf("%s", ssd->data[i]);
        } else {
            printf(" ");
        }
        if (i > 0 && (i + 1) % 10 == 0) {
            printf(" ");
        }
    }
    printf("\n");

    // Live
    printf("Live  ");
    for (int i = 0; i < ssd->num_pages; i++) {
        if (ssd->state[i] == STATE_VALID && ssd->forward_map[ssd->reverse_map[i]] == i) {
            printf("+");
        } else {
            printf(" ");
        }
        if (i > 0 && (i + 1) % 10 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}
//trim-------------------------------------------------------------------------------------------------
     char *trim(SSD *ssd, int address) {
    ssd->logical_trim_sum += 1;
    if (address < 0 || address >= ssd->num_logical_pages) {
        ssd->logical_trim_fail_sum += 1;
        return "fail: illegal trim address";
    }
    if (ssd->forward_map[address] == -1) {
        ssd->logical_trim_fail_sum += 1;
        return "fail: uninitialized trim";
    }
    ssd->forward_map[address] = -1;
    return "success";
}

//read--------------------------------------------------------------------------------------------------- 
     char *read(SSD *ssd, int address) {
    ssd->logical_read_sum += 1;
    if (address < 0 || address >= ssd->num_logical_pages) {
        ssd->logical_read_fail_sum += 1;
        return "fail: illegal read address";
    }
    if (ssd->forward_map[address] == -1) {
        ssd->logical_read_fail_sum += 1;
        return "fail: uninitialized read";
    }
    // Used for DIRECT, LOGGING, and IDEAL
    return read_direct(ssd, ssd->forward_map[address]);
}

//write-----------------------------------------------------------------------------
    char *write(SSD *ssd, int address,const char *data) {
    ssd->logical_write_sum += 1;
    if (address < 0 || address >= ssd->num_logical_pages) {
        ssd->logical_write_fail_sum += 1;
        return "fail: illegal write address";
    }
    if (ssd->ssd_type == TYPE_DIRECT) {
        return write_direct(ssd, address, data);
    } else if (ssd->ssd_type == TYPE_IDEAL) {
        return write_ideal(ssd, address, data);
    } else {
        return write_logging(ssd, address, data);
    }
}
//garbage collect-----------------------------------------------------------------------------------------------
void garbage_collect(SSD *ssd) {
    int blocks_cleaned = 0;

    // Flatten the iteration over two ranges: [gc_current_block, num_blocks) and [0, gc_current_block)
    for (int i = 0; i < ssd->num_blocks; i++) {
        int block = (ssd->gc_current_block + i) % ssd->num_blocks;

        // Skip the block currently being written to
        if (block == ssd->current_block) {
            continue;
        }

        // Start page for this block
        int page_start = block * ssd->pages_per_block;

        // Skip if the block is already erased
        if (ssd->state[page_start] == STATE_ERASED) {
            continue;
        }

        // Collect list of live physical pages in this block
        int live_pages[ssd->pages_per_block];
        int live_pages_count = 0;

        for (int page = page_start; page < page_start + ssd->pages_per_block; page++) {
            int logical_page = ssd->reverse_map[page];
            if (logical_page != -1 && ssd->forward_map[logical_page] == page) {
                live_pages[live_pages_count++] = page;
            }
        }

        // If all pages are live, skip cleaning this block
        if (live_pages_count == ssd->pages_per_block) {
            continue;
        }

        // Copy live pages to the current writing location
        for (int j = 0; j < live_pages_count; j++) {
            int page = live_pages[j];
            if (ssd->gc_trace) {
                printf("gc %d:: read(physical_page=%d)\n", ssd->gc_count, page);
                printf("gc %d:: write()\n", ssd->gc_count);
            }
            char *data = physical_read(ssd, page);  // Placeholder for physical_read
           char *result= write(ssd, ssd->reverse_map[page], data);  // Placeholder for write
        }

        // Erase the block
        blocks_cleaned++;
        physical_erase(ssd, block);  // Placeholder for physical_erase

        if (ssd->gc_trace) {
            printf("gc %d:: erase(block=%d)\n", ssd->gc_count, block);
            if (ssd->show_state) {
                printf("\n");
                dump(ssd);  // Placeholder for dump
                printf("\n");
            }
        }

        // Check if GC is complete
        if (blocks_in_use(ssd) <= ssd->gc_low_water_mark) {  // Placeholder for blocks_in_use
            ssd->gc_current_block = block;
            ssd->gc_count++;
            return;
        }
    }

    // End of block iteration
    return;
}
//upkeep-----------------------------------------------------------------------------------------------------------------
void upkeep(SSD *ssd) {
    // Garbage Collection
    if (blocks_in_use(ssd) >= ssd->gc_high_water_mark) {
        garbage_collect(ssd);
    }
    // Wear leveling can be added here in the future
}
//stats----------------------------------------------------------------------------------------------------------------
void stats(SSD *ssd) {
    printf("Physical Operations Per Block\n");
    
    // Erase stats
    printf("Erases ");
    for (int i = 0; i < ssd->num_blocks; i++) {
        printf("%3d        ", ssd->physical_erase_count[i]);
    }
    printf("  Sum: %d\n", ssd->physical_erase_sum);

    // Write stats
    printf("Writes ");
    for (int i = 0; i < ssd->num_blocks; i++) {
        printf("%3d        ", ssd->physical_write_count[i]);
    }
    printf("  Sum: %d\n", ssd->physical_write_sum);

    // Read stats
    printf("Reads  ");
    for (int i = 0; i < ssd->num_blocks; i++) {
        printf("%3d        ", ssd->physical_read_count[i]);
    }
    printf("  Sum: %d\n", ssd->physical_read_sum);

    printf("\nLogical Operation Sums\n");
    printf("  Write count %d (%d failed)\n", ssd->logical_write_sum, ssd->logical_write_fail_sum);
    printf("  Read count  %d (%d failed)\n", ssd->logical_read_sum, ssd->logical_read_fail_sum);
    printf("  Trim count  %d (%d failed)\n", ssd->logical_trim_sum, ssd->logical_trim_fail_sum);

    // Time calculations
    printf("\nTimes\n");
    printf("  Erase time %.2f\n", (float)(ssd->physical_erase_sum * ssd->block_erase_time));
    printf("  Write time %.2f\n", (float)(ssd->physical_write_sum * ssd->page_program_time));
    printf("  Read time  %.2f\n", (float)(ssd->physical_read_sum * ssd->page_read_time));
    double total_time = ssd->physical_erase_sum * ssd->block_erase_time +
                        ssd->physical_write_sum * ssd->page_program_time +
                        ssd->physical_read_sum * ssd->page_read_time;
    printf("  Total time %.2f\n", total_time);
}


// Function to free the SSD
void ssd_free(SSD *ssd) {
    free(ssd->state);
    for (int i = 0; i < ssd->num_pages; i++) {
        free(ssd->data[i]);
    }
    free(ssd->data);
    free(ssd->gc_used_blocks);
    free(ssd->live_count);
    free(ssd->forward_map);
    free(ssd->reverse_map);
    free(ssd->physical_erase_count);
    free(ssd->physical_read_count);
    free(ssd->physical_write_count);
    free(ssd);
}
void random_seed(int seed) {
    srand(seed); // Use seed for random number generation
}

//by hemil
void generate_printable_characters(char *printable) {
    int index = 0;

    // Add digits
    for (char c = '0'; c <= '9'; c++) {
        printable[index++] = c;
    }
    // Add lowercase letters
    for (char c = 'a'; c <= 'z'; c++) {
        printable[index++] = c;
    }
    // Add uppercase letters
    for (char c = 'A'; c <= 'Z'; c++) {
        printable[index++] = c;
    }

    printable[index] = '\0';  // Null terminate the string
}
int main(int argc, char *argv[]) {
    // Default values for options
    int seed = 0;
    int num_cmds = 10;
    char *op_percentages = "40/50/10";
    char *skew = "";
    int skew_start = 0;
    int read_fail = 0;
    char *cmd_list = "";
    char *ssd_type = "direct";
    int num_logical_pages = 50;
    int num_blocks = 7;
    int pages_per_block = 10;
    int high_water_mark = 10;
    int low_water_mark = 8;
    int read_time = 10;
    int program_time = 40;
    int erase_time = 1000;
    int show_gc = 0;
    int show_state = 0;
    int show_cmds = 0;
    int quiz_cmds = 0;
    int show_stats = 0;
    int compute = 0;

    // Define the long options
    struct option long_options[] = {
        {"seed", required_argument, 0, 's'},
        {"num_cmds", required_argument, 0, 'n'},
        {"op_percentages", required_argument, 0, 'P'},
        {"skew", required_argument, 0, 'K'},
        {"skew_start", required_argument, 0, 'k'},
        {"read_fails", required_argument, 0, 'r'},
        {"cmd_list", required_argument, 0, 'L'},
        {"ssd_type", required_argument, 0, 'T'},
        {"logical_pages", required_argument, 0, 'l'},
        {"num_blocks", required_argument, 0, 'B'},
        {"pages_per_block", required_argument, 0, 'p'},
        {"high_water_mark", required_argument, 0, 'G'},
        {"low_water_mark", required_argument, 0, 'g'},
        {"read_time", required_argument, 0, 'R'},
        {"program_time", required_argument, 0, 'W'},
        {"erase_time", required_argument, 0, 'E'},
        {"show_gc", no_argument,  0,'J'},
        {"show_state",no_argument,  0,'F'},
        {"show_cmds",no_argument,  0,'C'},
        {"quiz_cmds",no_argument, 0,'q'},
        {"show_stats",no_argument, 0,'S'},
        {"compute", no_argument, 0,'c'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    // Parse the command-line options
    while ((opt = getopt_long(argc, argv, "s:n:P:K:k:r:L:T:l:B:p:G:g:R:W:E:JFCqSc", long_options, &option_index)) != -1) {
        switch (opt) {
            case 's': seed = atoi(optarg); break;
            case 'n': num_cmds = atoi(optarg); break;
            case 'P': op_percentages = optarg; break;
            case 'K': skew = optarg; break;
            case 'k': skew_start = atoi(optarg); break;
            case 'r': read_fail = atoi(optarg); break;
            case 'L': cmd_list = optarg; break;
            case 'T': ssd_type = optarg; break;
            case 'l': num_logical_pages = atoi(optarg); break;
            case 'B': num_blocks = atoi(optarg); break;
            case 'p': pages_per_block = atoi(optarg); break;
            case 'G': high_water_mark = atoi(optarg); break;
            case 'g': low_water_mark = atoi(optarg); break;
            case 'R': read_time = atoi(optarg); break;
            case 'W': program_time = atoi(optarg); break;
            case 'E': erase_time = atoi(optarg); break;
            case 'J': show_gc = 1;break;//atoi(optarg); break;
            case 'F': show_state = 1;break;//atoi(optarg); break;
            case 'C': show_cmds = 1;break;//atoi(optarg); break;
            case 'q': quiz_cmds = 1;break;//atoi(optarg); break;
            case 'S': show_stats = 1;break;//atoi(optarg); break;
            case 'c': compute = 1;break;//atoi(optarg); break;
            case 0: break; // For options with no arguments
            default:
                fprintf(stderr, "Unknown option\n");
                exit(EXIT_FAILURE);
        }
    }

    // Call random_seed with the provided seed
    random_seed(seed);

    // Print the parsed options
    printf("ARG seed %d\n", seed);
    printf("ARG num_cmds %d\n", num_cmds);
    printf("ARG op_percentages %s\n", op_percentages);
    printf("ARG skew %s\n", skew);
    printf("ARG skew_start %d\n", skew_start);
    printf("ARG read_fail %d\n", read_fail);
    printf("ARG cmd_list %s\n", cmd_list);
    printf("ARG ssd_type %s\n", ssd_type);
    printf("ARG num_logical_pages %d\n", num_logical_pages);
    printf("ARG num_blocks %d\n", num_blocks);
    printf("ARG pages_per_block %d\n", pages_per_block);
    printf("ARG high_water_mark %d\n", high_water_mark);
    printf("ARG low_water_mark %d\n", low_water_mark);
    printf("ARG erase_time %d\n", erase_time);
    printf("ARG program_time %d\n", program_time);
    printf("ARG read_time %d\n", read_time);
    printf("ARG show_gc %d\n", show_gc);
    printf("ARG show_state %d\n", show_state);
    printf("ARG show_cmds %d\n", show_cmds);
    printf("ARG quiz_cmds %d\n", quiz_cmds);
    printf("ARG show_stats %d\n", show_stats);
    printf("ARG compute %d\n", compute);

    SSD *s = ssd_init(ssd_type, num_logical_pages, num_blocks, pages_per_block, erase_time, program_time, read_time,high_water_mark, low_water_mark, show_gc, show_state);

    int hot_cold = 0;int hot_percent;int hot_target;int skew_start_ = skew_start;
    if(strlen(skew)!=0){
        hot_cold=1;
        char skwno1[]={skew[0],skew[1],'\0'};
        char skwno2[]={skew[3],skew[4],'\0'};
        if(strlen(skew)!=5 || skew[2]!='/' || skwno1[0]>'9' || skwno1[1]<'0' || skwno2[0]>'9' || skwno2[1]<'0'){
            printf("bad skew specification; should be 80/20 or something like that\n");
            return 0;
        }
        int skew_no[2] = {atoi(skwno1),atoi(skwno2)};
        int hot_percent = skew_no[0]/100;
        int hot_target = skew_no[1]/100;
    }

    srand(time(0));
    //implimenting cmd_list 
    #define MAX_TOKENS 10  // Maximum number of tokens
    #define MAX_TOKEN_LEN 20  // Maximum length of each tok
    //declaring commeant struct
    int no_cmd_list=0;
    char c_cmd_list[100];
    int a_cmd_list[100];
    char *d_cmd_list[100];
    for(int j=0;j<100;j++){
        c_cmd_list[j]='?';
        a_cmd_list[j]=-1;
        d_cmd_list[j]="?";
    }


    if(strlen(cmd_list)==0){
        int length = -1;
        char pr[3][100] = { "", "", "" }; // Array to store tokens
        int index_r = 0, index_c = 0;              // Row and column indices

        // Parse the string
        while (length < (int)strlen(op_percentages)) {
            length++;
            if (op_percentages[length] == '/') {
                pr[index_c][index_r] = '\0'; // Null-terminate the current token
                index_r = 0;
                index_c++;
                continue;
            }
            pr[index_c][index_r++] = op_percentages[length];
        }
        pr[index_c][index_r] = '\0'; // Null-terminate the last token
        int percent_reads = atoi(pr[0]);
        int percent_writes = atoi(pr[1]);
        int percent_trims = atoi(pr[2]);//printf("%d\n%d\n%d\n",percent_reads,percent_writes,percent_trims);
            
            
        if(percent_writes<=0){
            printf("must have some writes, otherwise nothing in the SSD!\n");
            return 1;
        }


        char printable[63];  // Space for 62 characters + null terminator
        generate_printable_characters(printable);
            
            
            
        int valid_addresses[100];
        for(int j=0;j<100;j++){
            valid_addresses[j]=-1;
        }
        int sizeofvalid_address = 0;
        while (no_cmd_list<num_cmds){
            int which_cmd = rand()%100;int address;
               // printf("%d\t", which_cmd);
                //printf("%d",which_cmd);
            if(which_cmd<percent_reads){
                //READ
                //printf("Read\n");
                int rand_val = rand()%100;
                if(rand_val<read_fail){
                        address = rand()%num_logical_pages;
                    }else{
                        if(sizeofvalid_address<2){
                            continue;
                        }
                        address = valid_addresses[rand()%sizeofvalid_address];
                    }
                    c_cmd_list[no_cmd_list]='r';a_cmd_list[no_cmd_list]=address;
                    no_cmd_list++;
                }
                else if(which_cmd<percent_reads+percent_writes){
                    //WRITE
                    //printf("WRITE\n");
                    float rand_val = (float)rand() / RAND_MAX; // Random number between 0 and 1
                    if(skew_start_==0 && hot_cold && rand_val < hot_percent){
                        address = rand() % (int)(hot_target * (num_logical_pages - 1));
                    }else{
                        address = rand()%num_logical_pages;
                    }
                    //printf("%d\n",address);
                    //if address not in valid_addresses then append it
                    //printf("\n\n");
                    for(int i=0;i<100;i++){
                        if(valid_addresses[i]==address){
                            break;
                        }
                        if(valid_addresses[i]==-1){
                            valid_addresses[i]=address;sizeofvalid_address=i+1;
                            break;
                        }
                    }
                    char *random_char = (char *)malloc(2 * sizeof(char));  // Allocate memory for 1 char + '\0'
                    random_char[0] = printable[rand() % 62];  // Get a random character
                    random_char[1] = '\0';  // Null-terminate the string
                    // char random_char[2];  // Space for character + null terminator
                    //     random_char[0] = printable[rand() % 63];
                    //     random_char[1] = '\0';
                    //char data = printable[rand()%63];
                    c_cmd_list[no_cmd_list]='w';a_cmd_list[no_cmd_list]=address;d_cmd_list[no_cmd_list]=random_char;
                    //random_char[0]='\0';
                    no_cmd_list++;
                    if(skew_start_ >0){
                        skew_start_ -=1;
                    }
                }
                else{
                    //TRIM
                    //printf("TRIM\n");
                    if(sizeofvalid_address<1){
                        continue;            
                    }
                    address = valid_addresses[rand()%sizeofvalid_address];
                    //printf("\n%d\n",address);
                    c_cmd_list[no_cmd_list]='t';a_cmd_list[no_cmd_list]=address;
                    int cpy_valid_address[100];
                    for(int j=0;j<100;j++){
                        cpy_valid_address[j]=-1;
                    }
                    int ptr=0;
                    for(int i=0;i<100;i++){
                        if(valid_addresses[i]==address){
                            continue;
                        }
                        cpy_valid_address[ptr++]=valid_addresses[i];
                    }
                    for(int i=0;i<100;i++){
                        valid_addresses[i]=cpy_valid_address[i];
                    }
                    sizeofvalid_address--;
                    no_cmd_list++;
                }
                
                
            }//printf("\n---------\n");
            // for(int i=0;i<100;i++){
            //     if(valid_addresses[i]==-1){
            //         printf("over\n");break;
            //     }
            //     printf("%d ",valid_addresses[i]);
            // }
            // printf("\nsize of valid addresses %d\n",sizeofvalid_address);
            // printf("\n %read %d\t %trim %d\t %writes %d\n",percent_reads,percent_trims,percent_writes);
    }else{
        //printf("%s\n",cmd_list);
        char address[100];for(int j=0;j<100;j++){address[j]='\0';}
        int index = 0;
    while (cmd_list[index] != '\0') {
        if (cmd_list[index] == 'r') {
            // For read
            index++;
            int add_cnt = 0;
            while (cmd_list[index] != ',' && cmd_list[index] != '\0') {
                address[add_cnt++] = cmd_list[index++];
            }
            //printf("Read\t%s\n", address);
            c_cmd_list[no_cmd_list] = 'r';
            a_cmd_list[no_cmd_list] = atoi(address);
            //printf("Read\t%c\t%d\n", c_cmd_list[no_cmd_list], a_cmd_list[no_cmd_list]);
            no_cmd_list++;
            memset(address, '\0', sizeof(address));

        } else if (cmd_list[index] == 'w') {
            // For write
            index++;
            int add_cnt = 0;
            while (cmd_list[index] != ':' && cmd_list[index] != '\0') {
                address[add_cnt++] = cmd_list[index++];
            }
            index++; // Skip ':'
            char data[2];
            data[0] = cmd_list[index++];
            data[1] = '\0';

            // Allocate memory for the data and copy it to d_cmd_list
            char *data_copy = (char *)malloc(2 * sizeof(char));  // Allocate memory for 1 char + null terminator
            strcpy(data_copy, data);  // Copy the content of data to the allocated memory

            c_cmd_list[no_cmd_list] = 'w';
            a_cmd_list[no_cmd_list] = atoi(address);
            d_cmd_list[no_cmd_list] = data_copy;

            no_cmd_list++;
            memset(address, '\0', sizeof(address));


        } else if (cmd_list[index] == 't') {
            // For trim
            index++;
            int add_cnt = 0;
            while (cmd_list[index] != ',' && cmd_list[index] != '\0') {
                address[add_cnt++] = cmd_list[index++];
            }
            //printf("Trim\t%s\n", address);
            c_cmd_list[no_cmd_list] = 't';
            a_cmd_list[no_cmd_list] = atoi(address);
            //printf("Trim\t%c\t%d\n", c_cmd_list[no_cmd_list], a_cmd_list[no_cmd_list]);
            no_cmd_list++;
            memset(address, '\0', sizeof(address));

        } else {
            printf("Invalid command at index %d\n", index);
            break;
        }
        
        // Skip ',' if present
        if (cmd_list[index] == ',') {
            index++;
        }
    }

    }
    
    dump(s);printf("\n");
    // printf("Data string \t");for(int i=0;i<100;i++){
    //     printf("%s",d_cmd_list[i]);
    // }
    // printf("\n-----------------------\n");
    // for(int i=0;i<no_cmd_list;i++){
    //     //printf("i=%d\t",i);
    //     printf("%c",c_cmd_list[i]);
    //     printf("%d",a_cmd_list[i]);
    //     if(c_cmd_list[i]=='w'){
    //         printf(":%s",d_cmd_list[i]);
    //     }
    //     printf(" \t\t ");
    // }printf("\n");


    // printf("show_state:%d\tshow_cmds:%d\tquiz_cmds:%d\t\n",show_state,show_cmds,quiz_cmds);
    if(quiz_cmds){
        show_state = 1;
    }
    //printf("Show Commands %d\n",show_cmds);
    int op=0;int solve=compute;
    //printf("Number of commands :%d\n",no_cmd_list);
    for(int i=0;i<no_cmd_list;i++){
        //printf("Command :%c\t",c_cmd_list[i]);
        if(!(c_cmd_list[i]=='r' || c_cmd_list[i]=='w' || c_cmd_list[i]=='t')){
            //printf("Break time\n");
            break;
        }
        if(c_cmd_list[i]=='r'){
            //printf("Read\n");
            //implementing read
            const char *data=read(s,a_cmd_list[i]);
            //strcpy(data,read(s,a_cmd_list[i]));
            if(show_cmds || (quiz_cmds && solve)){
                printf("cmd %3d:: read(%d) -> %s\n",op,a_cmd_list[i],data);
            }else if(quiz_cmds){
                printf("cmd %3d:: read(%d) -> %c%c\n",op,a_cmd_list[i],'?','?');
            }
            op++;
        }else if(c_cmd_list[i]=='w'){
            //implementing write
            //printf("Write\n");
             //const char *data = d_cmd_list[i];
            const char *rc=write(s,a_cmd_list[i],d_cmd_list[i]);
            //strcpy(write(s,a_cmd_list[i],d_cmd_list[i]),rc);
            if(show_cmds || (quiz_cmds && solve)){
                //printf("Inside Show command\n");
                printf("cmd %3d:: write(%d, %s) -> %s\n",op,a_cmd_list[i],d_cmd_list[i],rc);
            }else if(quiz_cmds){
                printf("cmd %3d:: command(%c%c) -> %c%c\n",op,'?','?','?','?');
            
            }
            op++;
        }else if(c_cmd_list[i]=='t'){
            //printf("Trim\n");
            const char *rc=trim(s,a_cmd_list[i]); 
            //strcpy(trim(s,a_cmd_list[i]),rc);
            if(show_cmds || (quiz_cmds && solve)){
                printf("cmd %3d:: trim(%d) -> %s\n",op,a_cmd_list[i],rc);
            }else if(quiz_cmds){
                printf("cmd %d:: command(%c%c) -> %c%c\n",op,'?','?','?','?');
            }
            op++;
        }
        if(show_state){
            printf("\n");
            dump(s);
            printf("\n");
        }

        upkeep(s);
    }
    if(!show_state){
        printf("\n");
        dump(s);
        printf("\n");
    }
    if(show_stats){
        stats(s);
        printf("\n");
    }


    // printf("Read count: %d\tErease Count: %d\tWrite Count: %d\n",s->physical_read_sum,s->physical_erase_sum,s->physical_write_sum);
    // printf("Read : %d\tWrite:%d \tErease: %d\n",s->page_read_time,s->page_program_time,s->block_erase_time);
    ssd_free(s); // Free memory
    return 0;
}
