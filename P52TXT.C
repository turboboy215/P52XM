/*Paragon 5 (GBC) to XM converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long tablePtrLoc;
long tableOffset;
long patOffset;
int i, j;
char outfile[1000000];
int songNum;
long bankAmt;
long insPtr;
long songPtr;
int foundTable = 0;
int songBank = 0;
int insBank = 0;

unsigned static char* romData;
unsigned static char* songBankData;
unsigned static char* patBankData;

const char TableFind[4] = { 0xCE, 0x00, 0x67, 0x2A };
const char TableFindS11[9] = { 0xE0, 0x79, 0x6F, 0x26, 0x00, 0x29, 0x29, 0x29, 0x11 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);
void song2txtMB(int songNum, long ptr, int bank);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Paragon 5 (GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: P522TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			bankAmt = bankSize;

			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);

			if (bank > 1)
			{
				fclose(rom);
			}

			if (bank > 1)
			{
				/*Try to search the bank for song pattern table loader*/
				for (i = 0; i < bankSize; i++)
				{
					if (!memcmp(&romData[i], TableFind, 4))
					{
						tablePtrLoc = bankAmt + i - 4;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 3 - bankAmt] * 0x100);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
						break;
					}
				}

				if (foundTable == 1)
				{
					i = tableOffset - bankAmt;
					songNum = 1;

					while ((ReadLE16(&romData[i]) > bankAmt) && (ReadLE16(&romData[i + 2]) > bankAmt))
					{
						insPtr = ReadLE16(&romData[i]);
						songPtr = ReadLE16(&romData[i + 2]);
						printf("Song %i instruments: 0x%04X\n", songNum, insPtr);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2txt(songNum, songPtr);
						i += 4;
						songNum++;
					}
				}

				else
				{
					printf("ERROR: Magic bytes not found!\n");
					exit(-1);
				}
			}

			/*Multi-bank support version, used in Project S-11*/
			else if (bank == 1)
			{
				/*Try to search the bank for song pattern table loader*/
				for (i = 0; i < bankSize; i++)
				{
					if (!memcmp(&romData[i], TableFindS11, 9))
					{
						tablePtrLoc = i + 9;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = ReadLE16(&romData[tablePtrLoc]);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
						break;
					}
				}

				if (foundTable == 1)
				{
					i = tableOffset;
					songNum = 1;

					while ((ReadLE16(&romData[i + 2]) >= bankAmt) && (ReadLE16(&romData[i + 6]) >= bankAmt))
					{
						insBank = romData[i];
						insPtr = ReadLE16(&romData[i + 2]);
						songBank = romData[i + 4];
						songPtr = ReadLE16(&romData[i + 6]);
						printf("Song %i instruments bank: %01X\n", songNum, insBank);
						printf("Song %i instruments: 0x%04X\n", songNum, insPtr);
						printf("Song %i bank: %01X\n", songNum, songBank);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2txtMB(songNum, songPtr, songBank);
						printf("\n");
						i += 8;
						songNum++;
					}
					fclose(rom);
				}

				else
				{
					printf("ERROR: Magic bytes not found!\n");
					exit(-1);
				}
			}



		}
		printf("The operation was completed successfully!\n");
		return 0;
	}
}

/*Convert the song data to TXT*/
void song2txt(int songNum, long ptr)
{
	int songPos = 0;
	int patPos = 0;
	long curPat = 0;
	long curBank = 0;
	int patNum = 1;
	int patEnd = 0;
	long command[3];
	int curNote = 0;
	int curChan = 0;
	int curSpeed = 0;
	int curIns = 0;
	int tempVal = 0;
	int breakRows = 0;
	unsigned char lowNibble = 0;
	unsigned char highNibble = 0;

	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{
		songPos = ptr - bankAmt;

		while (ReadLE16(&romData[songPos]) > bankAmt)
		{
			curPat = ReadLE16(&romData[songPos]);
			curBank = ReadLE16(&romData[songPos + 2]);
			fprintf(txt, "Pattern #%i: 0x%04X, bank %02X\n", patNum, curPat, curBank);

			patPos = curPat - bankAmt;
			patEnd = 0;
			while (patEnd == 0)
			{
				command[0] = romData[patPos];
				command[1] = romData[patPos + 1];
				command[2] = romData[patPos + 2];

				/*Break pattern OR following rows are empty*/
				if (command[0] == 0x00)
				{
					if (command[2] == 0x07)
					{
						breakRows = ((command[1] + 1) / curSpeed) - 1;
						fprintf(txt, "Break pattern after %i rows\n", breakRows);
					}

					else if (command[1] < (curSpeed * 4))
					{
						fprintf(txt, "Change speed: %i\n", command[1]);
					}

					else
					{
						breakRows = ((command[1] + 1) / curSpeed) - 1;
						fprintf(txt, "Next %i rows are empty\n", breakRows);
					}
					fprintf(txt, "\n");
					patPos += 2;
				}

				/*Play note, instrument*/
				else if (command[0] == 0x01)
				{
					curNote = command[1];
					
					if (((command[2] - 4) % 4 == 0))
					{
						curIns = (command[2] - 4) / 4;
						curChan = 1;
					}

					else if (((command[2] - 5) % 4 == 0))
					{
						curIns = (command[2] - 5) / 4;
						curChan = 2;
					}

					else if (((command[2] - 6) % 4 == 0))
					{
						curIns = (command[2] - 6) / 4;
						curChan = 3;
					}

					else if (((command[2] - 3) % 4 == 0))
					{
						curIns = (command[2] - 3) / 4;
						curChan = 4;
					}
					
					fprintf(txt, "Play note on channel %i: %01X, instrument: %01X\n", curChan, curNote, curIns);
					patPos += 3;
				}

				/*Play instrument OR note end*/
				else if (command[0] == 0x02)
				{
					if (command[1] < 0x04)
					{
						curChan = command[1] + 1;
						fprintf(txt, "Note end on channel %i\n", curChan);
					}
					
					else if (command[1] >= 0x04)
					{
						if (((command[1] - 4) % 4 == 0))
						{
							curIns = (command[1] - 4) / 4;
							curChan = 1;
						}

						else if (((command[1] - 5) % 4 == 0))
						{
							curIns = (command[1] - 5) / 4;
							curChan = 2;
						}

						else if (((command[1] - 6) % 4 == 0))
						{
							curIns = (command[1] - 6) / 4;
							curChan = 3;
						}

						else if (((command[1] - 3) % 4 == 0))
						{
							curIns = (command[1] - 3) / 4;
							curChan = 4;
						}
						fprintf(txt, "Play instrument on channel %i: %01X\n", curChan, curIns);
					}
					
					patPos += 2;
				}

				/*Set starting volume of envelope*/
				else if (command[0] == 0x03)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					fprintf(txt, "Set starting volume of envelope on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Vibrato*/
				else if (command[0] == 0x04)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					fprintf(txt, "Vibrato on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Effects off*/
				else if (command[0] == 0x05)
				{
					curChan = command[1] + 1;

					fprintf(txt, "Effects off: channel %i\n", curChan);
					patPos += 2;
				}

				/*Flag command (for game-specific data)*/
				else if (command[0] == 0x06)
				{
					fprintf(txt, "Flag command: %i\n", command[1]);
					patPos += 2;
				}

				/*End of pattern*/
				else if (command[0] == 0x07)
				{
					fprintf(txt, "End of pattern\n");
					patEnd = 1;
				}

				/*End loop data*/
				else if (command[0] == 0x08)
				{
					fprintf(txt, "End loop data: %01X, %01X\n", command[1], command[2]);
					patPos += 3;
				}

				/*Go to order*/
				else if (command[0] == 0x09)
				{
					fprintf(txt, "End of song\n");
					patEnd = 1;
				}

				/*Pattern speed*/
				else if (command[0] == 0x0A)
				{
					curSpeed = command[1];
					fprintf(txt, "Pattern speed: %01X\n", curSpeed);
					patPos += 2;
				}

				/*End of row*/
				else if (command[0] == 0x0B)
				{
					fprintf(txt, "End of row\n\n");
					patPos++;
				}

				/*Next row is empty*/
				else if (command[0] == 0x0C)
				{
					fprintf(txt, "\nEmpty row\n\n");
					patPos++;
				}

				/*Next 2 rows are empty*/
				else if (command[0] == 0x0D)
				{
					fprintf(txt, "\nEmpty 2 rows\n\n");
					patPos++;
				}

				/*Next 3 rows are empty*/
				else if (command[0] == 0x0E)
				{
					fprintf(txt, "\nEmpty 3 rows\n\n");
					patPos++;
				}

				/*Pitch slide down*/
				else if (command[0] == 0x0F)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;
					fprintf(txt, "Pitch slide down on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Pitch slide up*/
				else if (command[0] == 0x10)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;
					fprintf(txt, "Pitch slide up on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Arpeggio with halftones X and Y*/
				else if (command[0] == 0x11)
				{
					curChan = command[2] + 1;
					fprintf(txt, "Arpeggio on channel %i: %01X\n", curChan, command[1]);
					patPos += 3;
				}

				/*Set pulse width OR set volume*/
				else if (command[0] == 0x12)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					if (curChan < 3)
					{
						fprintf(txt, "Set pulse width: %i\n", lowNibble);
					}
					
					else if (curChan == 3)
					{
						fprintf(txt, "Set volume (CH3): %i\n", lowNibble);
					}

					patPos += 2;
				}

				else if (command[0] == 0xFF)
				{
					patEnd = 1;
				}

				/*Unknown command*/
				else
				{
					fprintf(txt, "Unknown command %01X\n", command[0]);
					patPos += 2;
				}
			}
			fprintf(txt, "\n");

			songPos += 4;
			patNum++;
		}
		fclose(txt);
	}


}

/*Convert the song data to TXT - multi-bank version*/
void song2txtMB(int songNum, long ptr, int bank)
{
	int songPos = 0;
	int patPos = 0;
	long curPat = 0;
	long patBank = 0;
	long curBank = 0;
	int patNum = 1;
	int patEnd = 0;
	long command[3];
	int curNote = 0;
	int curChan = 0;
	int curSpeed = 0;
	int curIns = 0;
	int tempVal = 0;
	int breakRows = 0;
	unsigned char lowNibble = 0;
	unsigned char highNibble = 0;

	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{

		patBank = songBank;
		/*Get the current bank*/
		fseek(rom, ((patBank) * bankSize), SEEK_SET);
		songBankData = (unsigned char*)malloc(bankSize);
		fread(songBankData, 1, bankSize, rom);
		songPos = ptr - bankAmt;

		while (ReadLE16(&songBankData[songPos]) >= bankAmt)
		{
			curPat = ReadLE16(&songBankData[songPos]);
			curBank = ReadLE16(&songBankData[songPos + 2]);


			fprintf(txt, "Pattern #%i: 0x%04X, bank %02X\n", patNum, curPat, curBank);
			
			/*Copy pattern data over to memory from set bank*/
			fseek(rom, ((curBank) * bankSize), SEEK_SET);
			patBankData = (unsigned char*)malloc(bankSize);
			fread(patBankData, 1, bankSize, rom);

			patPos = curPat - bankAmt;
			patEnd = 0;
			while (patEnd == 0)
			{
				command[0] = patBankData[patPos];
				command[1] = patBankData[patPos + 1];
				command[2] = patBankData[patPos + 2];

				/*Break pattern OR following rows are empty*/
				if (command[0] == 0x00)
				{
					if (command[2] == 0x07)
					{
						breakRows = ((command[1] + 1) / curSpeed) - 1;
						fprintf(txt, "Break pattern after %i rows\n", breakRows);
					}

					else if (command[1] < (curSpeed * 4))
					{
						fprintf(txt, "Change speed: %i\n", command[1]);
					}

					else
					{
						breakRows = ((command[1] + 1) / curSpeed) - 1;
						fprintf(txt, "Next %i rows are empty\n", breakRows);
					}
					fprintf(txt, "\n");
					patPos += 2;
				}

				/*Play note, instrument*/
				else if (command[0] == 0x01)
				{
					curNote = command[1];

					if (((command[2] - 4) % 4 == 0))
					{
						curIns = (command[2] - 4) / 4;
						curChan = 1;
					}

					else if (((command[2] - 5) % 4 == 0))
					{
						curIns = (command[2] - 5) / 4;
						curChan = 2;
					}

					else if (((command[2] - 6) % 4 == 0))
					{
						curIns = (command[2] - 6) / 4;
						curChan = 3;
					}

					else if (((command[2] - 3) % 4 == 0))
					{
						curIns = (command[2] - 3) / 4;
						curChan = 4;
					}

					fprintf(txt, "Play note on channel %i: %01X, instrument: %01X\n", curChan, curNote, curIns);
					patPos += 3;
				}

				/*Play instrument OR note end*/
				else if (command[0] == 0x02)
				{
					if (command[1] < 0x04)
					{
						curChan = command[1] + 1;
						fprintf(txt, "Note end on channel %i\n", curChan);
					}

					else if (command[1] >= 0x04)
					{
						if (((command[1] - 4) % 4 == 0))
						{
							curIns = (command[1] - 4) / 4;
							curChan = 1;
						}

						else if (((command[1] - 5) % 4 == 0))
						{
							curIns = (command[1] - 5) / 4;
							curChan = 2;
						}

						else if (((command[1] - 6) % 4 == 0))
						{
							curIns = (command[1] - 6) / 4;
							curChan = 3;
						}

						else if (((command[1] - 3) % 4 == 0))
						{
							curIns = (command[1] - 3) / 4;
							curChan = 4;
						}
						fprintf(txt, "Play instrument on channel %i: %01X\n", curChan, curIns);
					}

					patPos += 2;
				}

				/*Set starting volume of envelope*/
				else if (command[0] == 0x03)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					fprintf(txt, "Set starting volume of envelope on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Vibrato*/
				else if (command[0] == 0x04)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					fprintf(txt, "Vibrato on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Effects off*/
				else if (command[0] == 0x05)
				{
					curChan = command[1] + 1;

					fprintf(txt, "Effects off: channel %i\n", curChan);
					patPos += 2;
				}

				/*Flag command (for game-specific data)*/
				else if (command[0] == 0x06)
				{
					fprintf(txt, "Flag command: %i\n", command[1]);
					patPos += 2;
				}

				/*End of pattern*/
				else if (command[0] == 0x07)
				{
					fprintf(txt, "End of pattern\n");
					patEnd = 1;
				}

				/*End loop data*/
				else if (command[0] == 0x08)
				{
					fprintf(txt, "End loop data: %01X, %01X\n", command[1], command[2]);
					patPos += 3;
				}

				/*Go to order*/
				else if (command[0] == 0x09)
				{
					fprintf(txt, "End of song\n");
					patEnd = 1;
				}

				/*Pattern speed*/
				else if (command[0] == 0x0A)
				{
					curSpeed = command[1];
					fprintf(txt, "Pattern speed: %01X\n", curSpeed);
					patPos += 2;
				}

				/*End of row*/
				else if (command[0] == 0x0B)
				{
					fprintf(txt, "End of row\n\n");
					patPos++;
				}

				/*Next row is empty*/
				else if (command[0] == 0x0C)
				{
					fprintf(txt, "\nEmpty row\n\n");
					patPos++;
				}

				/*Next 2 rows are empty*/
				else if (command[0] == 0x0D)
				{
					fprintf(txt, "\nEmpty 2 rows\n\n");
					patPos++;
				}

				/*Next 3 rows are empty*/
				else if (command[0] == 0x0E)
				{
					fprintf(txt, "\nEmpty 3 rows\n\n");
					patPos++;
				}

				/*Pitch slide down*/
				else if (command[0] == 0x0F)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;
					fprintf(txt, "Pitch slide down on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Pitch slide up*/
				else if (command[0] == 0x10)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;
					fprintf(txt, "Pitch slide up on channel %i: %i\n", curChan, lowNibble);
					patPos += 2;
				}

				/*Arpeggio with halftones X and Y*/
				else if (command[0] == 0x11)
				{
					curChan = command[2] + 1;
					fprintf(txt, "Arpeggio on channel %i: %01X\n", curChan, command[1]);
					patPos += 3;
				}

				/*Set pulse width OR set volume*/
				else if (command[0] == 0x12)
				{
					lowNibble = (command[1] >> 4);
					highNibble = (command[1] & 15);
					curChan = highNibble + 1;

					if (curChan < 3)
					{
						fprintf(txt, "Set pulse width: %i\n", lowNibble);
					}

					else if (curChan == 3)
					{
						fprintf(txt, "Set volume (CH3): %i\n", lowNibble);
					}

					patPos += 2;
				}

				else if (command[0] == 0xFF)
				{
					patEnd = 1;
				}

				/*Unknown command*/
				else
				{
					fprintf(txt, "Unknown command %01X\n", command[0]);
					patPos += 2;
				}
			}
			fprintf(txt, "\n");

			songPos += 4;
			patNum++;
		}
		fclose(txt);
	}


}