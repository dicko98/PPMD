#ifndef PPMD_CODER_HEADER_0F508BAB_5509_11d5_83F2_000102A39096
#define PPMD_CODER_HEADER_0F508BAB_5509_11d5_83F2_000102A39096

/** PPMD_Coder is a class to compress and uncompress a file using the very
sophisticated PPMD method. It is based on the public domain code by Dmitry 
Shkarin which is widely available. Please notice that only one file can
compressed, archiving several files can be done with the help of TAR.

There is no general rule which OrderSize to use. If you have highly repeatable 
data it's worth to use a high value but in general this should be avoided
because the overhead is slightly greater and would increase your output file
compared to smaller OrderSizes. Just play around with the values to see what
I mean.

The SubAllocatorSize is the size in MB which will be allocated dynamically
on the heap. Only use high values if you have enough memory.

You must include the "ppmd.lib" in your project!

Please send any bugs to:
Andreas Muegge <andreas.muegge@gmx.de> or <andreas@starke-muegge.de>

2001-30-05
*/

#include <iostream>
#include <fstream>
#include <exception>
#include <wtypes.h>
#include <stdio.h>
#include <string>
using namespace std;

// functions implemented in the static library "ppmd.lib"
BOOL  __stdcall StartSubAllocator(int SubAllocatorSize);
void  __stdcall StopSubAllocator();
DWORD __stdcall GetUsedMemory();

void __stdcall EncodeFile(ofstream & EncodedFile, 
						  ifstream & DecodedFile,
						  int MaxOrder);

void __stdcall DecodeFile(ofstream & DecodedFile, 
						  ifstream & EncodedFile,
						  int MaxOrder);

class PPMD_Coder
{
public:
	/** Parameters are:
	szIn		- file to read in
	szOut		- file to write compressed/uncompressed data to. If this
				  parameter is NULL the extension "ppm" will be added 
				  (Compression) or the original filename stored in the 
				  archive will be used (Decompression)
	nSub		- Memory to use for compression in MBytes (1..256)
	nOrder		- Ordersize (2..16)
	*/
	PPMD_Coder(const TCHAR* szIn, const TCHAR* szOut = NULL, 
		int nSub = 16, int nOrder = 6 ) 
		: m_nOrder(nOrder), m_nSubAllocatorSize(nSub),
          SIGNATURE("PPMD_A"), m_nMemory(0)
	{ m_szIn = szIn; m_szOut = szOut; }
	
	~PPMD_Coder() { }

	/** Play around with the order sizes. Higher values require more 
	overhead so you must find the right balance. Values between 4 and 6
	are good starting points. */
	int	OrderSize(int nOrder) 
	{
		m_nOrder = nOrder; 
		m_nOrder = max(nOrder, 2);	// make sure that nOrder > 0
		m_nOrder = min(nOrder, 16);	// make sure that nOrder <= 16
		return m_nOrder;
	}

	/** Specify how much memory (in MBytes) can be used. */
	int	SubAllocatorSize(int nSub)
	{
		m_nSubAllocatorSize = nSub;
		m_nSubAllocatorSize = max(m_nSubAllocatorSize, 1);
		m_nSubAllocatorSize = min(m_nSubAllocatorSize, 256);

		return m_nSubAllocatorSize;
	}

	/** Access functions for OrderSize and SubAllocatorSize */
	int OrderSize() { return m_nOrder; }
	int SubAllocatorSize() {return m_nSubAllocatorSize; }

	/** Compress the file, return FALSE if an error occured */
	bool	Compress();

	/** Uncompress file. */
	bool	Uncompress();


	/** Return the memory used. */
	void	GetMemoryUsage(DWORD & nMB, DWORD & nBytes)
	{	
		nMB		= m_nMemory >> 18;
		nBytes	=  (10U*(m_nMemory-(nMB << 18))+(1 << 17)) >> 18;
		if (nBytes == 10) 
		{ 
			nMB++;
			nBytes = 0; 
		}
	}

	/** Return size of input file */
	DWORD	GetInputSize() {return m_nInfileSize; }

	/** Return size of output file */
	DWORD	GetOutputSize() {return m_nOutfileSize; }

	/** Return compression ratio */
	float	GetRatio()
	{
		if (m_nInfileSize == 0 || m_nOutfileSize == 0)
			return 0.0;

		return (100.0 - float(m_nOutfileSize * 100.0 / m_nInfileSize) );
	}

	/** Return compression ratio */
	float	GetRatioUncompressed()
	{
		if (m_nInfileSize == 0 || m_nOutfileSize == 0)
			return 0.0;

		return (100.0 - float(m_nInfileSize * 100.0 / m_nOutfileSize) );
	}

private:
	const TCHAR*	m_szIn;
	std::string 	m_szOut;
	const char*		SIGNATURE;
	int				m_nOrder;
	int				m_nSubAllocatorSize;
	DWORD			m_nMemory;
	DWORD			m_nInfileSize;
	DWORD			m_nOutfileSize;
};


//////////////////////////////////////////////////////////////////////////
bool PPMD_Coder::Compress() 
{
	ifstream Infile(m_szIn, ios::in | ios::binary);
	if (!Infile.is_open() )
	{
		char szTemp[512];
		sprintf(szTemp, "Couldn't open file <%s>!", m_szIn);
		throw exception(szTemp); 
		return FALSE;
	}
	
	if (m_szOut.empty())
	{
		m_szOut = m_szIn;
		m_szOut += ".ppm";
	}
	
	ofstream Outfile(m_szOut.c_str(), ios::out | ios::trunc | ios::binary);
	if (!Outfile.is_open())
	{
		std::string szError = "Couldn't open file <";
		szError += m_szOut;
		szError += ">!";

		throw exception(szError.c_str()); 
		return FALSE;
	}
	
	Outfile.write(SIGNATURE, strlen(SIGNATURE));
	Outfile.put(static_cast<char> (m_nOrder) );
	Outfile.put(static_cast<char> (m_nSubAllocatorSize - 1) );
	
	// Write filename-length and filename of input file.
	// You could also store filedate and attributes here.
	int nLen = strlen(m_szIn);
	Outfile.write(reinterpret_cast<char*> (&nLen), sizeof(nLen) );
	Outfile.write(m_szIn, nLen);
	
	if (!StartSubAllocator(m_nSubAllocatorSize))
	{
		throw exception("Memory allocation for PPMD failed!"); 
		return FALSE;
	}
	
	EncodeFile(Outfile, Infile, m_nOrder);
	
	m_nMemory = GetUsedMemory();
	
	StopSubAllocator();
	
	m_nInfileSize = Infile.tellg();
	m_nOutfileSize = Outfile.tellp();
	
	Outfile.flush();
	Infile.close(); Outfile.close();
	
	return TRUE;
}


bool PPMD_Coder::Uncompress()
{
	ifstream Infile(m_szIn, ios::in | ios::binary);
	if (!Infile.is_open() )
	{
		char szTemp[512];
		sprintf(szTemp, "Couldn't open file <%s>!", m_szIn);
		throw exception(szTemp); 
		return FALSE;
	}
	
	char szTemp[8];
	Infile.read(szTemp, strlen(SIGNATURE));
	
	szTemp[6] = '\0';
	if (strcmp(szTemp, SIGNATURE) != 0)
	{
		// file not created by this program!
		throw exception("Wrong signature found, cannot uncompress file!");
		return FALSE;
	}
	
	m_nOrder = Infile.get();
	m_nSubAllocatorSize = Infile.get() +1;
	
	int nLen;
	Infile.read(reinterpret_cast<char*> (&nLen), sizeof(nLen) );
	
	char *szOutTemp = new char[nLen+1];
	Infile.read(szOutTemp, nLen);
	szOutTemp[nLen] = '\0';
	
	if (m_szOut.empty())
		m_szOut = szOutTemp;
	
	ofstream Outfile(m_szOut.c_str(), ios::out | ios::trunc | ios::binary);
	if (!Outfile.is_open())
	{
		delete szOutTemp;
		
		char szTemp[512];
		sprintf(szTemp, "Couldn't open file <%s>!", m_szOut);
		throw exception(szTemp); 
		return FALSE;
	}
	
	delete szOutTemp;
	
	if (!StartSubAllocator(m_nSubAllocatorSize))
	{
		throw exception("Memory allocation for PPMD failed!"); 
		return FALSE;
	}
	
	DecodeFile(Outfile, Infile, m_nOrder);
	
	m_nMemory = GetUsedMemory();
	
	StopSubAllocator();
	
	m_nInfileSize = Infile.tellg();
	m_nOutfileSize = Outfile.tellp();
	
	Outfile.flush();
	Infile.close(); Outfile.close();
	
	return TRUE;
}



#endif