#include "header.h"

#define		MAX_CODE	8192
#define		MAX_MEMORY	0x00007d00

void		readfile(s8* filename);
void		assembler();
void		filewrite();

void		skip_ws(s8* code);
u32		char_hex(s8** code);
u8		get_reg(s8** p);
u64		packed(u8 a, u8 b, u8 c, u8 d, u32 e);
Instruction	unpack(u64 var);



u64		opcodes[MAX_MEMORY / 8];
u64		current_addr = 0;


s8		code[MAX_CODE];

s8* 		filename;


s32
main(s32 argc, s8* argv[])
{
	if(argc < 2) return 1;
	readfile(argv[1]);
	assembler();
	filewrite();
	return 0;
}



void
readfile(s8* f)
{
	filename = f;
	s32 fd = open(f, O_RDONLY);
	if(fd < 0) return;
	s32 size = read(fd, code, MAX_CODE);
	if(size < 0) { close(fd); return; }
	close(fd);
	code[size] = 0;
}

void
assembler()
{
	s8* p = code;
	u8 type = 0, s1 = 0, s2 = 0, s3 = 0;
	u32 imm =0;
	skip_ws(p);
	while(*p)
	{
		type = 0, s1 = 0, s2 = 0, s3 = 0, imm = 0;
		switch(*p)
		{
			case '@':
			{
				p++;
				imm = char_hex(&p);
				if(*p == ':') p++;
				current_addr = imm / 8;
				imm = 0;
				break;
			}
			case '#':
			{
				p++;
				imm = char_hex(&p);
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '$':
			{
				p++;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '#': p++; imm = char_hex(&p); type = OPCODE_MOV_IMM; break;
					case '$': p++; s2 = get_reg(&p); type = OPCODE_MOV_REG; break;
					case '*': p++; if(*p == '$') { p++; s2 = get_reg(&p); type = OPCODE_MOV_REG_VAL; } else { imm = char_hex(&p); type = OPCODE_MOV_ADDR_VAL; } break;
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '*':
			{
				p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s1 = get_reg(&p);
						if(*p == '=') p++;
						switch(*p)
						{
							case '#': p++; imm = char_hex(&p); type = OPCODE_REG_VAL_IMM; break;
							case '$': p++; s2 = get_reg(&p); type = OPCODE_REG_VAL_REG; break;
							case '*': p++; if(*p == '$') { p++; s2 = get_reg(&p); type = OPCODE_REG_VAL_REG_VAL; } else { imm = char_hex(&p); type = OPCODE_REG_VAL_ADDR_VAL; } break;
						}
						break;
					}
					default:
					{
						imm = char_hex(&p);
						if(*p == '=') p++;
						switch(*p)
						{
							case '$': p++; s2 = get_reg(&p); type = OPCODE_ADDR_VAL_REG; break;
							case '*': p++; if(*p == '$') p++; s2= get_reg(&p); type = OPCODE_ADDR_VAL_REG_VAL;
						}
						break;
					}
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '+':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				if(*p == '$') p++;
				s2 = get_reg(&p);
				if(*p == ',') p++;
				switch(*p)
				{
					case '$': p++; s3 = get_reg(&p); type = OPCODE_ADD_REG_REG; break;
					case '#': p++; imm = char_hex(&p); type = OPCODE_ADD_REG_IMM; break;
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '-':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s2 = get_reg(&p);
						if(*p == ',') p++;
						switch(*p)
						{
							case '$': p++; s3 = get_reg(&p); type = OPCODE_SUB_REG_REG; break;
							case '#': p++; imm = char_hex(&p); type = OPCODE_SUB_REG_IMM; break;
						}
						break;
					}
					case '#': p++; imm = char_hex(&p); if(*p == ',') p+=2; s3 = get_reg(&p); type = OPCODE_SUB_IMM_REG; break;
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case 'x':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				if(*p == '$') p++;
				s2 = get_reg(&p);
				if(*p == ',') p++;
				switch(*p)
				{
					case '$': p++; s3 = get_reg(&p); type = OPCODE_MUL_REG_REG; break;
					case '#': p++; imm = char_hex(&p); type = OPCODE_MUL_REG_IMM; break;
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '/':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s2 = get_reg(&p);
						if(*p == ',') p++;
						switch(*p)
						{
							case '$': p++; s3 = get_reg(&p); type = OPCODE_DIV_REG_REG; break;
							case '#': p++; imm = char_hex(&p); type = OPCODE_DIV_REG_IMM; break;
						}
						break;
					}
					case '#': p++; imm = char_hex(&p); if(*p == ',') p+=2; s3 = get_reg(&p); type = OPCODE_DIV_IMM_REG; break;
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '<':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s2 = get_reg(&p);
						if(*p == ',') p++;
						if(*p == '$')
						{
							p++;
							s3 = get_reg(&p);
							type = OPCODE_LSHIFT_REG_REG;
						}
						else if(*p == '#')
						{
							p++;
							imm = char_hex(&p);
							type = OPCODE_LSHIFT_REG_IMM;
						}
						else p++;
						break;
					}
					case '#':
					{
						p++;
						imm = char_hex(&p);
						if(*p == ',') p++;
						if(*p == '$') p++;
						s3 = get_reg(&p);
						type = OPCODE_LSHIFT_IMM_REG;
						break;
					}
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '>':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s2 = get_reg(&p);
						if(*p == ',') p++;
						if(*p == '$')
						{
							p++;
							s3 = get_reg(&p);
							type = OPCODE_RSHIFT_REG_REG;
						}
						else if(*p == '#')
						{
							p++;
							imm = char_hex(&p);
							type = OPCODE_RSHIFT_REG_IMM;
						}
						else p++;
						break;
					}
					case '#':
					{
						p++;
						imm = char_hex(&p);
						if(*p == ',') p++;
						if(*p == '$') p++;
						s3 = get_reg(&p);
						type = OPCODE_RSHIFT_IMM_REG;
						break;
					}
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '|':
			{
				p+=2;
				s1 = get_reg(&p);
				if(*p == '=') p++;
				switch(*p)
				{
					case '$':
					{
						p++;
						s2 = get_reg(&p);
						if(*p == ',') p++;
						if(*p == '$')
						{
							p++;
							s3 = get_reg(&p);
							type = OPCODE_OR_REG_REG;
						}
						else if(*p == '#')
						{
							p++;
							imm = char_hex(&p);
							type = OPCODE_OR_REG_IMM;
						}
						else p++;
						break;
					}
					case '#':
					{
						p++;
						imm = char_hex(&p);
						if(*p == ',') p++;
						if(*p == '$') p++;
						s3 = get_reg(&p);
						type = OPCODE_OR_IMM_REG;
						break;
					}
				}
				if(*p == ';') p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			case '&':
			{
				p++;
				break;
			}
			case '^':
			{
				p++;
				break;
			}
			case '!':
			{
				p++;
				switch(*p)
				{
					case '@': p++; imm = char_hex(&p); type = OPCODE_JMP_ADDR; break;
					case '$': p++; s3 = get_reg(&p); type = OPCODE_JMP_REG; break;
					case '#': p++; if(*p == '0') type = OPCODE_HALT; else if(*p == '1') type = OPCODE_RET; p++; break;
					case '(':
					{
						p++;
						break;
					}
				}
				if(*p == ';'); p++;
				opcodes[current_addr++] = packed(type, s1, s2, s3, imm);
				break;
			}
			default: p++; break;
		}
	}
}

void
filewrite()
{
	int i = 0;
	s8 buffer[32];
	while(*filename!='\0' && *filename != '.')
	buffer[i++] = *filename++;
	buffer[i++] = '.';
	buffer[i++] = 'j';
	buffer[i++] = 'b';
	buffer[i++] = 'i';
	buffer[i++] = 'n';
	buffer[i] = '\0';
	FILE* f = fopen(buffer, "w");
	if(!f) return;
	for(u32 i=0; i<MAX_MEMORY/ 8; i++)
	{
		{
			Instruction a = unpack(opcodes[i]);
			fprintf(f, "%02x%02x%02x%02x%08x;\n", a.opcode, a.r1, a.r2, a.r3, a.imm);
		}
	}
	fclose(f);
}

void
skip_ws(s8* code)
{
	u32 i = 0, j = 0;
	while(code[i])
	{
		if(code[i] != ' ' && code[i] != '\t' &&  code[i] != '\n' && code[i] != '\r') code[j++] = code[i];
		i++;
	}
	code[j] = 0;
}


u32
char_hex(s8** p)
{
	u32 value = 0;

	while
	(
		(**p >= '0' && **p <= '9') ||
		(**p >= 'A' && **p <= 'F') ||
		(**p >= 'a' && **p <= 'f')
	)
	{
		value <<= 4;
		if(**p >= '0' && **p <= '9')		value |= **p - '0';
		else if(**p >= 'A' && **p <= 'F')	value |= 10 + (**p - 'A');
		else if(**p >= 'a' && **p <= 'f')	value |= 10 + (**p - 'a');
		(*p)++;
	}
	return value;
}

u8
get_reg(s8** p)
{
	if(**p < '0' || **p > '9') return 0xff;
	u8 value = (u8)(**p - '0');
	(*p)++;
	return value;
}

u64
packed(u8 opcode, u8 s1, u8 s2, u8 s3, u32 imm)
{
	u64 val = 0x0000000000000000;
	val |= ((u64)opcode << 56);
	val |= ((u64)s1 << 48);
	val |= ((u64)s2 << 40);
	val |= ((u64)s3 << 32);
	val |= ((u64)imm << 0);
	return val;
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
