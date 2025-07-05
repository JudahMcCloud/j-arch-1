#include "header.h"

#define		MAX_MEMORY	0x00007d00
#define		MAX_REGISTER	9
#define		MAX_SENSORS	4

void 		readfile(s8* filename);
void 		emulator();
void 		filewrite(s8* filename);
void		sensor_update();
u64		sensor_out(u32 i);

Instruction	unpack(u64 val);

u64		mem[MAX_MEMORY/8];
u64		pc = 0;
u64		sp = 0;
u64		lr = 0;

u64		reg[MAX_REGISTER];

u8		sys_timer = 0;
u64		sensors[MAX_SENSORS];

s32
main(s32 argc, s8* argv[])
{
	if(argc < 2) return 1;
	readfile(argv[1]);
	emulator();
	filewrite(argv[1]);
	return 0;
}

void
readfile(s8* f)
{
	FILE* file = fopen(f, "r");
	if(!file) return;
	size_t index_ = 0;
	u64 value = 0;
	s32 count = 0;
	s32 c;
	while((c = fgetc(file)) != EOF)
	{
		if(c == ';')
		{
			if(index_ >= MAX_MEMORY / 8) break;
			mem[index_] = value;
			index_++;
			value = 0;
			count = 0;
			c = fgetc(file);
			if(c != '\n') return;
		}
		else if(c == '\n' || c == ' ' || c == '\t' || c == '\r')
		continue;
		else
		{
			value <<= 4;
			if(c >= '0' && c <= '9') value |= (c - '0');
			else if(c >= 'a' && c <= 'f') value |= (c - 'a'+10);
			else { fprintf(stderr, "Invalid hex char\n"); exit(1); }
			count++;
		}
	}
	fclose(file);
}

void
emulator()
{
	s32 running = 1;
	while(running)
	{
		Instruction in = unpack(mem[pc++]);
		switch(in.opcode)
		{
			case OPCODE_MOV_IMM:		reg[in.r1] = 	in.imm; break;
			case OPCODE_MOV_REG:		reg[in.r1] = 	reg[in.r2]; break;
			case OPCODE_MOV_REG_VAL:
			{
				if(reg[in.r2] == SYS_TIMER_BASE_ADDR) 	mem[reg[in.r2]/8] = sys_timer;
				reg[in.r1] = 				mem[reg[in.r2]/8];
				break;
			}
			case OPCODE_MOV_ADDR_VAL:
			{
				if(in.imm == SYS_TIMER_BASE_ADDR) 	mem[in.imm/8] = sys_timer;
				reg[in.r1] = 				mem[in.imm/8];
				break;
			}
			case OPCODE_ADDR_VAL_REG:
			{
				mem[in.imm/8] = 			reg[in.r2];
				break;
			}
			case OPCODE_ADDR_VAL_REG_VAL:
			{
				if(reg[in.r2] == I_BASE_ADDR) 		mem[reg[in.r2]/8] = (u64)getchar();
				else if(reg[in.r2] == SYS_TIMER_BASE_ADDR) 	mem[reg[in.r2]/8] = sys_timer;
				mem[in.imm/8] = 			mem[reg[in.r2]/8];
				break;
			}
			case OPCODE_REG_VAL_IMM:
			{
				mem[reg[in.r1]/8] = 			in.imm;
				break;
			}
			case OPCODE_REG_VAL_REG:
			{
				mem[reg[in.r1]/8] = 			reg[in.r2];
				break;
			}
			case OPCODE_REG_VAL_REG_VAL:
			{
				if(reg[in.r2] == I_BASE_ADDR) 		mem[reg[in.r2]/8] = (u64)getchar();
				else if(reg[in.r2] == SYS_TIMER_BASE_ADDR) 	mem[reg[in.r2]/8] = sys_timer;
				mem[reg[in.r1]/8] = 			mem[reg[in.r2]/8];
				break;
			}
			case OPCODE_REG_VAL_ADDR_VAL:
			{
				if(in.imm == I_BASE_ADDR) 		mem[in.imm/8] = (u64)getchar();
				else if(in.imm == SYS_TIMER_BASE_ADDR) 	mem[in.imm/8] = sys_timer;
				mem[reg[in.r1]/8] = 			mem[in.imm/8];
				break;
			}
			case OPCODE_ADD_REG_IMM:	reg[in.r1] = reg[in.r2] + in.imm; break;
			case OPCODE_ADD_REG_REG:	reg[in.r1] = reg[in.r2] + reg[in.r3]; break;

			case OPCODE_SUB_IMM_REG:	reg[in.r1] = in.imm - reg[in.r3]; break;
			case OPCODE_SUB_REG_IMM:	reg[in.r1] = reg[in.r2] - in.imm; break;
			case OPCODE_SUB_REG_REG:	reg[in.r1] = reg[in.r2] - reg[in.r3]; break;

			case OPCODE_MUL_REG_IMM:	reg[in.r1] = reg[in.r2] * in.imm; break;
			case OPCODE_MUL_REG_REG:	reg[in.r1] = reg[in.r2] * reg[in.r3]; break;

			case OPCODE_DIV_IMM_REG:	reg[in.r1] = in.imm / reg[in.r3]; break;
			case OPCODE_DIV_REG_IMM:	reg[in.r1] = reg[in.r2] / in.imm; break;
			case OPCODE_DIV_REG_REG:	reg[in.r1] = reg[in.r2] * reg[in.r3]; break;

			case OPCODE_LSHIFT_REG_IMM:	reg[in.r1] = reg[in.r2] << in.imm; break;
			case OPCODE_LSHIFT_IMM_REG: 	reg[in.r1] = in.imm << reg[in.r3]; break;
			case OPCODE_LSHIFT_REG_REG: 	reg[in.r1] = reg[in.r2] << reg[in.r3]; break;

			case OPCODE_RSHIFT_REG_IMM:	reg[in.r1] = reg[in.r2] >> in.imm; break;
			case OPCODE_RSHIFT_IMM_REG: 	reg[in.r1] = in.imm >> reg[in.r3]; break;
			case OPCODE_RSHIFT_REG_REG: 	reg[in.r1] = reg[in.r2] >> reg[in.r3]; break;

			case OPCODE_OR_REG_IMM:		reg[in.r1] = reg[in.r2] | in.imm; break;
			case OPCODE_OR_IMM_REG: 	reg[in.r1] = in.imm | reg[in.r3]; break;
			case OPCODE_OR_REG_REG: 	reg[in.r1] = reg[in.r2] | reg[in.r3]; break;


			case OPCODE_CMP_EQ_ADDR:	if(reg[in.r1] == reg[in.r2]) pc = in.imm / 8; break;
			case OPCODE_CMP_NE_ADDR:	if(reg[in.r1] != reg[in.r2]) pc = in.imm / 8; break;
			case OPCODE_CMP_GT_ADDR:	if(reg[in.r1] > reg[in.r2]) pc = in.imm / 8; break;
			case OPCODE_CMP_LT_ADDR:	if(reg[in.r1] < reg[in.r2]) pc = in.imm / 8; break;

			case OPCODE_CMP_GTE_ADDR:	if(reg[in.r1] >= reg[in.r2]) pc = in.imm / 8; break;
			case OPCODE_CMP_LTE_ADDR:	if(reg[in.r1] <= reg[in.r2]) pc = in.imm / 8; break;

			case OPCODE_JMP_ADDR:		lr = pc; pc = in.imm / 8; break;
			case OPCODE_JMP_REG:		lr = pc; pc = reg[in.r2] / 8; break;
			case OPCODE_RET:		pc = lr; break;
			case OPCODE_HALT:		running = 0; break;
		}
		sys_timer++;
		sensor_update();
	}
}

void
filewrite(s8* filename)
{
	FILE* f = fopen(filename, "w");
	if(!f) return;
	for(u32 i=0; i<MAX_MEMORY/ 8; i++)
	{
		{
			Instruction a = unpack(mem[i]);
			fprintf(f, "%02x%02x%02x%02x%08x;\n", a.opcode, a.r1, a.r2, a.r3, a.imm);
		}
	}
	fclose(f);
}


Instruction
unpack(u64 val)
{
	Instruction a;
	a.opcode = (val >> 56) & 0xff;
	a.r1 = (val >> 48) & 0xff;
	a.r2 = (val >> 40) & 0xff;
	a.r3 = (val >> 32) & 0xff;
	a.imm = (val >> 0) & 0xffffffff;
	return a;
}

u64
sensor_out(u32 i)
{
	if(i >= MAX_SENSORS) return 0;
	return sensors[i];
}

void
sensor_update()
{
	sensors[0] = (sensors[0] + 1) % 256;
	sensors[1] = rand() % 500;
	sensors[2] = (sensors[2] - 1) % 254;
	sensors[3] = rand() % 499;
}
