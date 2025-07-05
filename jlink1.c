#include "header.h"

#define		MAX_MEMORY	0x00007d00

void		read_all_files_togather(s8* list[], s32 argc);

u64		packed(u8 a, u8 b, u8 c, u8 d, u32 e);
Instruction	unpack(u64 val);

u64		mem[MAX_MEMORY/8];
size_t index_ = 0;

s32
main(s32 argc, s8* argv[])
{
	if(argc < 2) return 1;
	read_all_files_togather(argv, argc);
	return 0;
}

void
read_all_files_togather(s8* list[], s32 argc)
{
	FILE* dest = fopen("program.jbin", "w");
	if(!dest) { fprintf(stderr, "Porgram file not created\n"); exit(1); }
	for(int i=1; i<argc; i++)
	{
		FILE* file = fopen(list[i], "r");
		if(!file) return;

		index_ = 0;
		u64 value = 0;
		s32 count = 0;

		s32 c;
		while((c = fgetc(file)) != EOF)
		{
			if(c == ';')
			{
				if(index_ >= MAX_MEMORY / 8) break;
				if(value != 0x0000000000000000) mem[index_] = value;
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
	for(u32 i=0; i<MAX_MEMORY/ 8; i++)
	{
		{
			Instruction a = unpack(mem[i]);
			fprintf(dest, "%02x%02x%02x%02x%08x;\n", a.opcode, a.r1, a.r2, a.r3, a.imm);
		}
	}
	fclose(dest);

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
