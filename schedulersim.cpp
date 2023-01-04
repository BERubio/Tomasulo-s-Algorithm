#include "schedulersim.hpp"
#include <cstdio>
#include <tuple>

int current_cycle = 1;

/**
 * Subroutine that returns how long it takes to complete a specific operation type
 *
 * @return                      Instruction latency in cycles
 */
int get_inst_latency(op_type op) {
    if(op == OP_ADD) {
        return 2;
    } else if(op == OP_DIV) {
        return 15;
    } else if(op == OP_MEM) {
        return 20;
    }
    std::printf("Invalid OP type:%d\n", op);
    std::exit(1);
}

/**
 * You are welcome to (re)define and set any global classes and variables as needed.
 */

int num_regs;

class RAT {
    // register alias table
    private:
        int* table;
    public:
        void init_table() {
            table = (int*) calloc(num_regs, sizeof(int));
            for (int i = 0; i < num_regs; i++) {
                table[i] = -1;
            }
        };;
        void free_table() {
            free(table);
        };
        void set_reg(int reg_num, int val) {
            table[reg_num - 1] = val;
        };
        int get_reg(int reg_num) {
            return table[reg_num - 1];
        };
};

RAT rat;

class REST {
    // reservation station
    private:
        int size;
        struct REST_Entry* table;
    public:
        void init_table(int table_size, int index_start) {
            size = table_size;
            table = (struct REST_Entry*) calloc(size, sizeof(struct REST_Entry));
            for (int i = 0; i < size; i++) {
                table[i].valid = false;
                table[i].rat_index = index_start++;
            }
            //printf("init_table ran");
        };
        void free_table() {
            free(table);
        };
        int get_size() {
            return size;
        };
        bool add_entry(op_type op, int dest, int src1, int src2, scheduler_stats_t* p_stats) {
            // put the instruction in the free entry of REST
            for (int i = 1; i <= size; i++) {
                struct REST_Entry* entry = &(table[i - 1]);

                if (!is_valid(i)) {
                    //how do I actually create an entry???
                    // set start cycle to zero and set rat index with reg_num and val------> what to use for val????
                    // Invalid Pointer error is coming from this function!!!!
                    entry->valid = true;
                    entry->op = op;
                    entry->dest = dest;
                    entry->src1 = src1;
                    entry->src2 = src2;
                    entry->start_cycle = 0;
                    //Update RAT: set register number and update REST rat index
                    //what does the rat_index get set to? 
                    rat.set_reg(dest, entry->rat_index);
                    //entry->rat_index = rat.get_reg(dest);

                    //provided:
                    p_stats->num_insts++;
                    //printf("Add_entry ran");
                    return true;
                }
            }
            p_stats->issue_stall++;
            return false;
        };
        void clear_entry(int entry_num) {
            table[entry_num - 1].valid = false;
        };
        bool is_valid(int entry_num) {
            return table[entry_num - 1].valid;
        };
        bool is_empty() {
            // check if there are any valid entries in the table
            for(int i = 1; i <= size; i++){
                if(is_valid(i)){
                    return false;
                }
            }
            return true;
        };
        int fire_ready() {
            int num_fired = 0;
            bool a_fired = false;
            bool d_fired = false;
            bool m_fired = false;
            for (int i = 1; i <= size; i++) {
                struct REST_Entry* e = &(table[i - 1]);
                // start executing (fire) any valid instruction in the table with ready sources
                // note: we marked the ready sources with -1
                /*
                 * (4.1.) Add your code here
                 */
                //check if valid, operands ready, and start cycle is set to zero
                if(is_valid(i) && e->src1 == -1 && e->src2 == -1 && e->start_cycle == 0){
                    
                    if(e->op == OP_ADD && a_fired == false){
                        e->start_cycle = current_cycle;
                        num_fired++;
                        a_fired = true;
                    }else{
                        if(e->op == OP_DIV && d_fired == false){
                            e->start_cycle = current_cycle;
                            num_fired++;
                            d_fired = true;
                        }else{
                            if(e->op == OP_MEM && m_fired == false){
                                e->start_cycle = current_cycle;
                                num_fired++;
                                m_fired = true;
                            }
                        }
                    }
                }
            }
            //printf("fire_ready ran")
            return num_fired;
        };
        int complete_insts() {
            int num_completed = 0;
            for (int i = 1; i <= size; i++) {
                struct REST_Entry* e = &(table[i - 1]);
                // for every instruction in the table check if its execution is done
                // if yes, then write the results back
                // feel free to call the broadcast_complete here
                /*
                 * (5.1.) Add your code here
                 */
                if(e->valid && e->start_cycle != 0){
                    switch (e->op) {
                        case OP_ADD:
                            if((current_cycle - get_inst_latency(OP_ADD)) == (e->start_cycle)){
                                rat.set_reg(e->dest, -1);
                                clear_entry(i);
                                broadcast_complete(e->rat_index);
                                num_completed++;
                            }
                            break;
                        case OP_DIV:
                            if((current_cycle - get_inst_latency(OP_DIV)) == (e->start_cycle)){
                                rat.set_reg(e->dest, -1);
                                clear_entry(i);
                                broadcast_complete(e->rat_index);
                                num_completed++;
                            }
                            break;
                        case OP_MEM:
                            if((current_cycle - get_inst_latency(OP_MEM)) == (e->start_cycle)){
                                rat.set_reg(e->dest, -1);
                                clear_entry(i);
                                broadcast_complete(e->rat_index);
                                num_completed++;
                            }
                            break;
                        default:
                            break;
                    }
                } 
            }
            
            //printf("complete_insts ran");
            return num_completed;
        };
        int count_active() {
            // Only to be used for a REST specific to a single FU - see below for unified
            int num_active = 0;
            for (int i = 1; i <= size; i++) {
                if (table[i - 1].valid && (table[i - 1].start_cycle > 0)) {
                    num_active++;
                }
            }
            return num_active;
        };
        std::tuple<int, int, int> count_active_u() {
            // To be used for a unified REST - see above for "per FU" REST
            int num_active_a = 0;
            int num_active_d = 0;
            int num_active_m = 0;
            for (int i = 1; i <= size; i++) {
                if (table[i - 1].valid && (table[i - 1].start_cycle > 0)) {
                    switch (table[i - 1].op) {
                        case OP_ADD:
                            num_active_a++;
                            break;
                        case OP_DIV:
                            num_active_d++;
                            break;
                        case OP_MEM:
                            num_active_m++;
                            break;
                        default:
                            printf("Error: Invalid op_type: %d", table[i - 1].op);
                            break;
                    }
                }
            }
            return std::make_tuple(num_active_a, num_active_d, num_active_m);
        }
        void update_sources(int rat_index) {
            for (int i = 1; i <= size; i++) {
                struct REST_Entry* e = &(table[i - 1]);
                if (e->valid) {
                    // we mark the match sources with -1
                    if (e->src1 == rat_index) {
                        e->src1 = -1;
                    }
                    if (e->src2 == rat_index) {
                        e->src2 = -1;
                    }
                }
            }
          //  printf("update_sources ran");
        }
};

// Reservation stations - unified, add, div, and mem
// Note some of these will be unused depending on unified vs per FU
REST urest;
REST arest;
REST drest;
REST mrest;

rs_type rs;

void broadcast_complete(int rat_index) {
    if (rs == 'U') {
        urest.update_sources(rat_index);
    } else {
        arest.update_sources(rat_index);
        drest.update_sources(rat_index);
        mrest.update_sources(rat_index);
    }
    //printf("broadcast_compelte ran");
}


/**
 * Subroutine for initializing the scheduler (unified reservation station type).
 * You may initalize any global or heap variables as needed.
 *
 * @param[in]   num_registers   The number of registers in the instructions
 * @param[in]   rs_size         The number of entries for the unified RS
 */
void scheduler_unified_init(int num_registers, int rs_size) {
    num_regs = num_registers;
    rat.init_table();
    rs = RSTYPE_UNIFIED;
    urest.init_table(rs_size, 1);
    //printf("scheduler_unified_init ran");
}

/**
 * Subroutine for initializing the scheduler (per-functional unit reservation station type).
 * You may initalize any global or heap variables as needed.
 *
 * (1) You're responsible for completing this routine
 *
 * @param[in]   num_registers   The number of registers in the instructions
 * @param[in]   rs_sizes        An array of size 3 that contains the number of entries for each
 *                              op_type
 *                              rs_sizes = [4,2,1] means 4 ADD RS, 2 DIV RS, 1 MEM RS
 */
void scheduler_per_fu_init(int num_registers, int rs_sizes[]) {
  num_regs = num_registers;
  rat.init_table();
  rs = RSTYPE_PER_FU;
  //What value do i use for start_index in rest.init_table(size, start_index)? Doing num_reg - rs_sizes[x] could produce equivalent values!

  arest.init_table(rs_sizes[0], 1);
  drest.init_table(rs_sizes[1], (rs_sizes[0]+1));
  mrest.init_table(rs_sizes[2], (rs_sizes[0]+rs_sizes[1]+1));
  //printf("scheduler_per_fu_init ran");
}

/**
 * Subroutine that tries to issue an instruction to the reservation station. You need to
 * choose the appropriate RS depending on the RS type and op_type and update it.
 *
 * (2) You're responsible for completing this routine and
 * the add_entry routine marked with (2.1.) above
 *
 * @param[in]   op_type         The FU to use
 * @param[in]   dest            The destination register
 * @param[in]   src1            The first source register (-1 if unused)
 * @param[in]   src2            The seconde source register (-1 if unused)
 * @param[out]  p_stats         Pointer to the stats structure
 *
 * @return                      true if successful, false if we failed
 */
bool scheduler_try_issue(op_type op, int dest, int src1, int src2, scheduler_stats_t* p_stats) {
    int alias1 = src1;
    if (src1 != -1) {
        alias1 = rat.get_reg(src1);
    }
    int alias2 = src2;
    if (src2 != -1) {
        alias2 = rat.get_reg(src2);
    }
    if (rs == 'U') {
      return urest.add_entry(op, dest, alias1, alias2, p_stats);
    } else {
        //if rs isn't unified then check for other open functional units
        //need to check whether RS are open for an entry---> this is done by add_entry function!!!
        //after adding entry, update p_stats: number of instrs or number of issue stalls---> done by add_entry function!!!
      p_stats->num_cycles++;
      switch (op) {
        case OP_ADD:
            return arest.add_entry(op, dest, alias1, alias2, p_stats);            
        case OP_DIV:
            return drest.add_entry(op, dest, alias1, alias2, p_stats);       
        case OP_MEM:
            return mrest.add_entry(op, dest, alias1, alias2, p_stats);
        default:
            printf("Error: Invalid op_type: %d", op);
            break;
      }
      //if(op == OP_ADD){
       // printf("scheduler_try_issue ADD ran");
        
      //}
      //if(op == OP_DIV){
        //printf("scheduler_try_issue DIV ran");
        
      //}
      //f(op == OP_MEM){
        //printf("scheduler_try_issue MEM ran");
      //}
      //When do I update the number of cycles?
      
    }
    return false;
}

/**
 * Subroutine that checks if all instructions have been drained from the pipeline
 * (3) You're responsible for completing this routine and
 * the is_empty routine marked with (3.1.) above
 *
 * @return                      true if no instructions are left
 */
bool scheduler_completed() {
    if (rs == 'U') {
      return urest.is_empty();
    } else {
      /*
       * (3.2.) Add your code here
       */
       if(arest.is_empty() && drest.is_empty() && mrest.is_empty()){
            //printf("scheduler_completed true ran");
            return true;
       }else{
            //printf("scheduler_completed false ran");
            return false;
       }
    }
    return false;
}

/**
 * Subroutine that increments the clock cycle and updates any system state
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_step(scheduler_stats_t* p_stats) {
    current_cycle++;
    scheduler_clear_completed(p_stats);
    scheduler_start_ready(p_stats);
}

/**
 * Subroutine for firing (start executing) any ready instructions.
 *
 * (4) You're responsible for completing this routine and
 * the fire_ready routine marked with (4.1.) above
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_start_ready(scheduler_stats_t* p_stats) {
    unsigned int num_fired = 0;
    if (rs == 'U') {
      num_fired += urest.fire_ready();
    } else {
      /*
       * (4.2.) Add your code here
       */
     //  if(rs == 'A'){
            num_fired += arest.fire_ready();
      // }
      // if(rs == 'D'){
            num_fired += drest.fire_ready();
       //}
       //if(rs == 'M'){
            num_fired += mrest.fire_ready();
      // }
    }
    // Update p_stats with number of fired instructions
    /*
     * (4.3.) Add your code here
     */
    if(num_fired > (p_stats->max_fired)){
        p_stats->max_fired = num_fired;
    }
    // Count num active per FU
    unsigned int num_active_a;
    unsigned int num_active_d;
    unsigned int num_active_m;

    if (rs == 'U') {
        auto t = urest.count_active_u();
        num_active_a = std::get<0>(t);
        num_active_d = std::get<1>(t);
        num_active_m = std::get<2>(t);
    } else {
      num_active_a = arest.count_active();
      num_active_d = drest.count_active();
      num_active_m = mrest.count_active();
    }

    // Update pstats with number of active FUs
    /*
     * (4.4.) Add your code here
     */
    if(num_active_a > (p_stats->max_active[0])){
        p_stats->max_active[0] = num_active_a;
    }
    if(num_active_d > (p_stats->max_active[1])){
        p_stats->max_active[1] = num_active_d;
    }
    if(num_active_m > (p_stats->max_active[2])){
        p_stats->max_active[2] = num_active_m;
    }
   // printf("scheduler_start_ready ran");
}

/**
 * Subroutine for clearing any completed instructions.
 * (5) You're responsible for completing this routine and
 * the complete_insts routine marked with (5.1.) above
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_clear_completed(scheduler_stats_t* p_stats) {
    unsigned int num_completed = 0;

    if (rs == 'U') {
      num_completed += urest.complete_insts();
    } else {
      /*
       * (5.2.) Add your code here
       */
       //if instruction is complete--> how do I know???
       //complete_insts will determine whether an instr. is finished.        
       num_completed += arest.complete_insts();
       num_completed += drest.complete_insts();
       num_completed += mrest.complete_insts();
    }

    // Update p_stats with number of completed instructions
    /*
     * (5.3.) Add your code here
     */
    if(num_completed > p_stats->max_completed){
        p_stats->max_completed = num_completed;
    }
    //printf("scheduler_clear_completed ran");
}

/**
 * Subroutine for completing the scheduler and getting any final stats
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_complete(scheduler_stats_t* p_stats) {
    // Free memory
    if (rs == 'U') {
        urest.free_table();
    } else {
        arest.free_table();
        drest.free_table();
        mrest.free_table();
    }
    rat.free_table();

    // Update number of cycles and ipc in p_stats
    p_stats->num_cycles = current_cycle;
    p_stats->ipc = (double) p_stats->num_insts / p_stats->num_cycles;
   // printf("scheduler_complete ran");
}
