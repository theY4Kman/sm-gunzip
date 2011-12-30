/**
 * =============================================================================
 * Yak's gunzip
 *  - gzip format based off http://www.gzip.org/zlib/rfc-gzip.html
 *  - inflate algorithm ported from Mark Adler's puff.c (zlib/contrib/puff)
 * Copyright (C) 2011 Zach "theY4Kman" Kanzler
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// =============================================================================
//  Gzip member header defines
// =============================================================================

#define GZ_ID1  0x1f    // Magic
#define GZ_ID2  0x8b    // Number

#define GZ_CM_STORED        0
#define GZ_CM_COMPRESSED    1
#define GZ_CM_PACKED        2
#define GZ_CM_LZHED         3
#define GZ_CM_DEFLATE       8   // The DEFLATE compression method

#define GZ_FLG_FTEXT        (1<<0)  // File is probably ASCII text
#define GZ_FLG_FHCRC        (1<<1)  // A CRC16 of the gzip header is present
#define GZ_FLG_FEXTRA       (1<<2)  // Optional extra fields are present
#define GZ_FLG_FNAME        (1<<3)  // Original filename is present
#define GZ_FLG_FCOMMENT     (1<<4)  // A file comment is present

#define GZ_MTIME_UNAVAILABLE    0   // Timestamp compression started is unavailable

#define GZ_DEFLATE_XFL_SLOWEST  2   // Maximum compression, slowest algorithm
#define GZ_DEFLATE_XFL_MEDIUM   3
#define GZ_DEFLATE_XFL_FASTEST  4   // Fastest algorithm

#define GZ_OS_FAT           0   // FAT filesystem (MS-DOS, OS/2, NT/Win32)
#define GZ_OS_AMIGA         1   // Amiga
#define GZ_OS_VMS           2   // VMS (or OpenVMS)
#define GZ_OS_UNIX          3   // Unix
#define GZ_OS_VMCMS         4   // VM/CMS
#define GZ_OS_ATARITOS      5   // Atari TOS
#define GZ_OS_HPFS          6   // HPFS filesystem (OS/2, NT)
#define GZ_OS_MACINTOSH     7   // Macintosh
#define GZ_OS_ZSYSTEM       8   // Z-System
#define GZ_OS_CPM           9   // CP/M
#define GZ_OS_TOPS20        10  // TOPS-20
#define GZ_OS_NTFS          11  // NTFS filesystem (NT)
#define GZ_OS_QDOS          12  // QDOS
#define GZ_OS_ACORN         13  // Acorn RISCOS
#define GZ_OS_UNKNOWN       255 // Unknown


// =============================================================================
//  INFLATE-related defines
// =============================================================================

#define GZ_LAST_BLOCK       1

#define GZ_BLOCK_STORED     0
#define GZ_BLOCK_FIXED      1
#define GZ_BLOCK_DYNAMIC    2
#define GZ_BLOCK_RESERVED   3

#define GZ_MAXBITS      15          /* max bits in a code */
#define GZ_MAXLCODES    286         /* max number of literal/length codes */
#define GZ_MAXDCODES    30          /* max number of distance codes */
#define GZ_MAXCODES     (GZ_MAXLCODES+GZ_MAXDCODES)
#define GZ_FIXLCODES    288         /* number of fixed literal/length codes */


// =============================================================================
//  Library defines
// =============================================================================

#define GZ_MAX_COMMENT_LENGTH   1024

#define GZ_STATE_ERROR  0
#define GZ_STATE_OUT    1
#define GZ_STATE_IN     2
#define GZ_STATE_BITBUF 3
#define GZ_STATE_BITCNT 4
#define GZ_STATE_SIZE   5

#define GZ_FLAG(%1,%2) (((%1) & (%2)) == (%2))

#define GZ_STATE        _state
#define GZ_STATE_DEF    GZ_STATE[GZ_STATE_SIZE]

#define GZ_HUFF_COUNT   huff_count
#define GZ_HUFF_SYMBOL  huff_symbol
#define GZ_HUFF_DEF     GZ_HUFF_COUNT[], GZ_HUFF_SYMBOL[]


// =============================================================================
//  Error codes (see GetErrorMessage for messages)
// =============================================================================

#define GZ_ERROR_SUCCESS                0000

// 1-999: gzip errors
#define GZ_ERROR_BAD_MEMBER_HEADER      0001
#define GZ_ERROR_UNIMPLEMENTED_CM       0002
#define GZ_ERROR_BAD_BLOCK              0003

// 1000-1999: INFLATE-related errors
#define GZ_ERROR_HUFF_OUT_OF_CODES      1000
#define GZ_ERROR_INVALID_FIXED_CODE     1001
#define GZ_ERROR_HUFF_BAD_COUNTS        1002
#define GZ_ERROR_REQUIRE_COMPLETE_SET   1003
#define GZ_ERROR_TOO_MANY_LENGTHS       1004
#define GZ_ERROR_NO_END_OF_BLOCK_CODE   1005
#define GZ_ERROR_INCOMPLETE_ONLY_ONE    1006
#define GZ_ERROR_NO_LAST_LENGTH         1007

// >= 9000: library errors
#define GZ_ERROR_END_OF_FILE            9000
#define GZ_ERROR_INPUT_FILE_BAD         9001
#define GZ_ERROR_OUTPUT_FILE_BAD        9002
#define GZ_ERROR_NOT_IMPLEMENTED        9003


// =============================================================================
//  Plug-in facing functions (use these)
// =============================================================================

/**
 * Extracts a gzip-compressed file `gzipped_file` to path `out_file`
 *
 * @param   gzipped_file    The path to the gzip-compressed file
 * @param   out_file        The path to the output file
 * @param   error           A buffer to write a human-readable version of the
 *                          error to, if there is an error.
 * @param   maxlength       The max number of chars to write to the buffer
 * @param   errcode         A reference to a variable to hold the unique error
 *                          code of the error, if there was one.
 * @note    If there are multiple entries in the gzipped file, they will be
 *          extracted to out_file.1, out_file.2, ..., out_file.N
 * @return  Number of files written on success, -1 on error
 */
stock ExtractGzipFile(const String:gzipped_file[], const String:out_file[],
                      String:error[], maxlength, &errcode)
{

    
    // Open the input gzipped file
    new Handle:gzfp = OpenFile(gzipped_file, "rb");
    if (gzfp == INVALID_HANDLE)
    {
        errcode = GZ_ERROR_INPUT_FILE_BAD;
        GetErrorMessage(GZ_ERROR_INPUT_FILE_BAD, error, maxlength);
        return -1;
    }
    
    // Loop over all the member entries in the gzip file
    new current_member = 0;
    while (!IsEndOfFile(gzfp))
    {
        #define __GZ_ERROR(%1) \
        { \
            if (current_member != 0) break; \
            errcode = %1; \
            GetErrorMessage(%1, error, maxlength); \
            Format(error, maxlength, "%s in member %d", error, \
                   current_member); \
            CloseHandle(gzfp); \
            return -1; \
        }

        new GZ_STATE_DEF;
        GZ_STATE[GZ_STATE_IN] = _:gzfp;
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_SUCCESS;
        
        // Member header declarations
        new member_id1      = 0,
            member_id2      = 0,
            member_cm       = 0,
            member_flg      = 0,
            member_mtime    = 0,
            member_xfl      = 0,
            member_os       = 0,
            member_crc32    = 0,
            member_isize    = 0;
        
        new member_xlen     = 0;
        new member_crc16    = 0;
        
        decl String:member_fname[PLATFORM_MAX_PATH];
        decl String:member_fcomment[GZ_MAX_COMMENT_LENGTH];
        
        decl membuf[4];
        
        // Read the magic number
        if (ReadFile(gzfp, membuf, 2, 1) != 2)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        
        member_id1 = membuf[0];
        member_id2 = membuf[1];
        
        // Check for correct magic number
        if (member_id1 != GZ_ID1 || member_id2 != GZ_ID2)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        
        // Read the compression method
        if (ReadFile(gzfp, membuf, 1, 1) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_cm = membuf[0];
        
        if (member_cm != GZ_CM_DEFLATE)
            __GZ_ERROR(GZ_ERROR_UNIMPLEMENTED_CM)
        
        // Read the member flags
        if (ReadFile(gzfp, membuf, 1, 1) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_flg = membuf[0];
        
        // Read in the timestamp compression started
        if (ReadFile(gzfp, membuf, 1, 4) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_mtime = membuf[0];
        
        // Read in the XFL field (tells us about the speed and rigorousness
        // of the compression method)
        if (ReadFile(gzfp, membuf, 1, 1) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_xfl = membuf[0];
        
        // Read the OS field, which describes the file system on which the
        // compression took place
        if (ReadFile(gzfp, membuf, 1, 1) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_os = membuf[0];
        
        // Handle extra fields if the FEXTRA flag is set
        if (GZ_FLAG(member_flg, GZ_FLG_FEXTRA))
        {
            if (ReadFile(gzfp, membuf, 1, 1) != 1)
                __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
            member_xlen = membuf[0];
            
            // If we cared about extra fields, we'd do something here
            FileSeek(gzfp, member_xlen, SEEK_CUR);
        }
        
        // Extract the original file name if FNAME is set
        if (GZ_FLAG(member_flg, GZ_FLG_FNAME))
        {
            if (ReadFileString(gzfp, member_fname, sizeof(member_fname)) == -1)
                __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        }

        // Extract the file comment if FCOMMENT is set
        if (GZ_FLAG(member_flg, GZ_FLG_FCOMMENT))
        {
            if (ReadFileString(gzfp, member_fcomment, sizeof(member_fcomment)) == -1)
                __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        }

        // Read the CRC16 if FHCRC is set
        if (GZ_FLAG(member_flg, GZ_FLG_FHCRC))
        {
            if (ReadFile(gzfp, membuf, 1, 2) != 1)
                __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
            member_crc16 = membuf[0];
        }
        
        // FOR DEBUGGING PURPOSES ONLY
        _gz_PrintMemberToServer(member_id1, member_id2, member_cm, member_flg,
                                member_mtime, member_xfl, member_os,
                                member_crc32, member_isize, member_xlen,
                                member_fname, member_fcomment, member_crc16);
        
        // Open the output file
        if (current_member > 0)
        {
            new len = strlen(out_file) + 12;
            decl String:inc_fname[len];
            Format(inc_fname, len, "%s.%d", out_file, current_member);
            GZ_STATE[GZ_STATE_OUT] = _:OpenFile(inc_fname, "wb");
        }
        else
            GZ_STATE[GZ_STATE_OUT] = _:OpenFile(out_file, "wb");
        
        // Make sure we can write to the output file
        if (Handle:GZ_STATE[GZ_STATE_OUT] == INVALID_HANDLE)
            __GZ_ERROR(GZ_ERROR_OUTPUT_FILE_BAD)
        
        // Loop through all the compressed blocks
        new True = true;
        new current_block = 0;
        while (True)
        {
            #undef __GZ_ERROR
            #define __GZ_ERROR(%1) \
            { \
                errcode = %1; \
                GetErrorMessage(%1, error, maxlength); \
                Format(error, maxlength, "%s in member %d, block %d", error, \
                       current_member, current_block); \
                CloseHandle(gzfp); \
                CloseHandle(Handle:GZ_STATE[GZ_STATE_OUT]); \
                return -1; \
            }
            
            new last_block = _gz_bits(_state, 1);
            if (last_block == -1 && GZ_STATE[GZ_STATE_ERROR])
                __GZ_ERROR(GZ_STATE[GZ_STATE_ERROR])
                        
            new block_type = _gz_bits(_state, 2);
            if (block_type == -1 && _state[GZ_STATE_ERROR])
                __GZ_ERROR(GZ_STATE[GZ_STATE_ERROR])
            
            decl ret;
            switch (block_type)
            {
                case GZ_BLOCK_STORED: ret = _gz_stored(_state);
                case GZ_BLOCK_FIXED: ret = _gz_fixed(_state);
                case GZ_BLOCK_DYNAMIC: ret = _gz_dynamic(_state);
                default: __GZ_ERROR(GZ_ERROR_BAD_BLOCK)
            }
            
            if (ret == -1)
                __GZ_ERROR(GZ_STATE[GZ_STATE_ERROR])
            
            CloseHandle(Handle:GZ_STATE[GZ_STATE_OUT]);
            
            if (last_block)
                break;
            current_block++;
        }

        // Read in the crc32 of the uncompressed member data
        if (ReadFile(gzfp, membuf, 1, 4) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_crc32 = membuf[0];

        // Read in the size of the uncompressed member data
        if (ReadFile(gzfp, membuf, 1, 4) != 1)
            __GZ_ERROR(GZ_ERROR_BAD_MEMBER_HEADER)
        member_isize = membuf[0];
        
        current_member++;
    }

    CloseHandle(gzfp);
    
    errcode = 0;
    strcopy(error, maxlength, "");
    return current_member;
}

/**
 * Get the human-readable version of an error code.
 *
 * @param   errcode     The error code
 * @param   message     A buffer to hold the error message
 * @param   maxlength   The max number of chars to write to the buffer
 * @return  The number of characters written to the buffer.
 */
stock GetErrorMessage(errcode, String:message[], maxlength)
{
    #define __GZ_ERRCPY(%1) return strcopy(message, maxlength, %1)
    #define GZERR(%1,%2) case (%1): __GZ_ERRCPY(%2)
    switch (errcode)
    {
        GZERR(GZ_ERROR_SUCCESS,"Success");
        
        // Gzip errors
        GZERR(GZ_ERROR_BAD_MEMBER_HEADER,"Bad member header");
        GZERR(GZ_ERROR_UNIMPLEMENTED_CM,"Compression method not implemented");
        GZERR(GZ_ERROR_BAD_BLOCK,"Bad compressed block");
        
        // INFLATE-related errors
        GZERR(GZ_ERROR_HUFF_OUT_OF_CODES,"Huffman decode: ran out of codes");
        GZERR(GZ_ERROR_INVALID_FIXED_CODE,"Invalid fixed code");
        GZERR(GZ_ERROR_HUFF_BAD_COUNTS,"Bad counts in dynamic block");
        GZERR(GZ_ERROR_REQUIRE_COMPLETE_SET,"Require complete code set");
        GZERR(GZ_ERROR_TOO_MANY_LENGTHS,"Too many lengths");
        GZERR(GZ_ERROR_NO_END_OF_BLOCK_CODE,"No end-of-block code");
        GZERR(GZ_ERROR_INCOMPLETE_ONLY_ONE,"Incomplete codes only allowed if just one code");
        GZERR(GZ_ERROR_NO_LAST_LENGTH,"");
        
        // Library errors
        GZERR(GZ_ERROR_END_OF_FILE,"EOF reached unexpectedly");
        GZERR(GZ_ERROR_INPUT_FILE_BAD,"Could not open input file");
        GZERR(GZ_ERROR_OUTPUT_FILE_BAD,"Could not open output file");
        GZERR(GZ_ERROR_NOT_IMPLEMENTED,"Feature not implemented");
    }
    
    __GZ_ERRCPY("Not an error");
}


// =============================================================================
//  Internal library-use functions, forward compatability not guaranteed 
// =============================================================================

stock _gz_PrintMemberToServer(id1, id2, cm, flg, mtime, xfl, os, crc32, isize,
                           xlen, const String:fname[], const String:fcomment[],
                           crc16)
{
#if defined(DEBUG)
    return;
#endif
    
    LogToGame("ID1: %02x", id1);
    LogToGame("ID2: %02x", id2);
    
    LogToGame("CM: %d%s", cm, (cm == GZ_CM_DEFLATE) ? " (DEFLATE)" : "");
    
#define     __GZ_FLG(%1) ((flg & (1<<(%1))) >> (%1))
    LogToGame("Flags:");
    LogToGame("   FTEXT    - %d", __GZ_FLG(0));
    LogToGame("   FHCRC    - %d", __GZ_FLG(1));
    LogToGame("   FEXTRA   - %d", __GZ_FLG(2));
    LogToGame("   FNAME    - %d", __GZ_FLG(3));
    LogToGame("   FCOMMENT - %d", __GZ_FLG(4));
    LogToGame("   reserved - %d", __GZ_FLG(5));
    LogToGame("   reserved - %d", __GZ_FLG(6));
    LogToGame("   reserved - %d", __GZ_FLG(7));
    
    if (GZ_FLAG(flg, GZ_FLG_FNAME))
        LogToGame("FNAME: \"%s\"", fname);
    if (GZ_FLAG(flg, GZ_FLG_FCOMMENT))
        LogToGame("FCOMMENT: \"%s\"", fcomment);
}

#define _GZ_BITS(%1,%2) \
    %1 = _gz_bits(_state, (%2)); \
    if (%1 == -1 && GZ_STATE[GZ_STATE_ERROR]) \
        return -1

stock _gz_bits(GZ_STATE_DEF, need)
{
    ////////////////////////////////////////////////////////////////////////
    LogToGame("bits(%d)", need, FilePosition(Handle:GZ_STATE[GZ_STATE_IN]));
    
    decl val;
    
    // Load at least `need` bits into val
    val = GZ_STATE[GZ_STATE_BITBUF];
    while (GZ_STATE[GZ_STATE_BITCNT] < need)
    {
        new byte[1];
        /* Load eight bits */
        if (ReadFile(Handle:GZ_STATE[GZ_STATE_IN], byte, 1, 1) != 1)
        {
            /* Out of input */
            GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
            return -1;
        }

        /////////////////////////////////////////////////////////////////////////
        new String:bitbuf[] = "        ";
        new String:valstr[] = "        ";
        for (new i=7; i>=0; i--)
        {
            bitbuf[7-i] = '0' + ((GZ_STATE[GZ_STATE_BITBUF] & (1<<i)) >> i);
            valstr[7-i] = '0' + ((val & (1<<i)) >> i);
        }
        LogToGame("  %-10s%-10s%-10s(%d)", "preread", valstr, bitbuf, GZ_STATE[GZ_STATE_BITCNT]);
        
        val |= byte[0] << GZ_STATE[GZ_STATE_BITCNT];
        GZ_STATE[GZ_STATE_BITCNT] += 8;
        
        /////////////////////////////////////////////////////////////////////////
        for (new i=7; i>=0; i--)
        {
            bitbuf[7-i] = '0' + ((GZ_STATE[GZ_STATE_BITBUF] & (1<<i)) >> i);
            valstr[7-i] = '0' + ((val & (1<<i)) >> i);
        }
        LogToGame("  %-10s%-10s%-10s(%d)", "postread", valstr, bitbuf, GZ_STATE[GZ_STATE_BITCNT]);
    }
    
    /* Drop `need` bits and update buffer, always zero to seven bits left */
    GZ_STATE[GZ_STATE_BITBUF] = (val >> need) & 0xFF;
    GZ_STATE[GZ_STATE_BITCNT] -= need;
    
    /* Return `need` bits, zeroing the bits above that */
    new bits = val & ((1 << need) - 1);
    
    /////////////////////////////////////////
    new String:bin[] = "        ";
    new String:bitbuf[] = "        ";
    new String:valstr[] = "        ";
    for (new i=7; i>=0; i--)
    {
        bin[7-i] = i == need ? ' ' : '0' + ((bits & (1<<i)) >> i);
        bitbuf[7-i] = '0' + ((GZ_STATE[GZ_STATE_BITBUF] & (1<<i)) >> i);
        valstr[7-i] = '0' + ((val & (1<<i)) >> i);
    }
    LogToGame("  %-10s%-10s%-10s(%-4d) (value: %d)", bin, valstr, bitbuf,
              GZ_STATE[GZ_STATE_BITCNT], bits);
    ////////////////////////////////////////////////////////////////
    
    return bits;
}

#define _GZ_BYTE(%1) \
    %1 = _gz_byte(_state, false); \
    if (%1 == -1 && _state[GZ_STATE_ERROR]) \
        return -1

#define _GZ_BYTE_PEEK(%1) \
    %1 = _gz_byte(_state, true); \
    if (%1 == -1 && _state[GZ_STATE_ERROR]) \
        return -1

stock _gz_byte(GZ_STATE_DEF, peek)
{
    //////////////////////////////////////////////////////////
    LogToGame("byte at %d", FilePosition(Handle:GZ_STATE[GZ_STATE_IN]));
    
    decl buf[1];
    decl loc;
    
    if (peek)
        loc = FilePosition(Handle:GZ_STATE[GZ_STATE_IN]);
    
    new ret = ReadFile(Handle:GZ_STATE[GZ_STATE_IN], buf, 1, 1);
    if (ret != 1)
    {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INPUT_FILE_BAD;
        if (ret != -1)
            GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
        return -1;
    }
    
    if (peek)
        FileSeek(Handle:GZ_STATE[GZ_STATE_IN], loc, SEEK_SET);
    
    return buf[0];
} 

#define _GZ_BYTE_GET(%1,%2,%3,%4) \
    %1 = _gz_byte_get(_state, (%2), (%3), (%4)); \
    if (%1 == -1 && _state[GZ_STATE_ERROR]) \
        return -1

stock _gz_byte_get(GZ_STATE_DEF, loc, seek, allow_invalid_distance)
{
    /////////////////////////////////////////////////////////////////////////////
    LogToGame("byte_get(%d,%d) at %d", loc, seek,
              FilePosition(Handle:GZ_STATE[GZ_STATE_IN]));////////////////////////////////////
    
    // Initialize buf to 0
    new buf[1];
    
    new orig_loc = FilePosition(Handle:GZ_STATE[GZ_STATE_IN]);
    FileSeek(Handle:GZ_STATE[GZ_STATE_IN], loc, seek);

    new ret = ReadFile(Handle:GZ_STATE[GZ_STATE_IN], buf, 1, 1);
    if (ret != 1 && !allow_invalid_distance)
    {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INPUT_FILE_BAD;
        if (ret != -1)
            GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
        return -1;
    }

    FileSeek(Handle:GZ_STATE[GZ_STATE_IN], orig_loc, SEEK_SET);
    return buf[0];
}

#define SLOW // FOR DEBUGGING PURPOSES ONLY
#if defined(SLOW)
stock _gz_decode(GZ_STATE_DEF, GZ_HUFF_DEF)
{LogToGame("decode");////////////////////////////////////
    decl len,   /* current number of bits in code */
         count; /* number of codes of length `len` */
    new  code,  /* `len` bits being decoded */
         first, /* first code of length `len` */
         index; /* index of first code of length `len` in symbol table */
    
    for (len=1; len <= GZ_MAXBITS; len++)
    {
        decl bits;
        _GZ_BITS(bits,1);
        
        /* get next bit */
        code |= bits;
        count = GZ_HUFF_COUNT[len];
        
        if (code - count < first)       /* if length `len`, return symbol */
            return GZ_HUFF_SYMBOL[index + (code - first)];
        index += count;                 /* else update for next length */
        first += count;
        first <<= 1;
        code <<= 1;
    }
    
    GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_HUFF_OUT_OF_CODES;
    return -1;                          /* ran out of codes */
}
#endif

stock _gz_construct(GZ_HUFF_DEF, length[], n)
{LogToGame("construct");////////////////////////////////////
    decl symbol;    /* current symbol when stepping through length[] */
    decl len;       /* current length when stepping through GZ_HUFF_COUNT */
    decl left;      /* number of possible codes left of current length */
    decl offs[GZ_MAXBITS + 1];  /* offsets in symbol table for each length */
    
    /* count number of codes of each length */
    for (len = 0; len <= GZ_MAXBITS; len++)
        GZ_HUFF_COUNT[len] = 0;
    for (symbol = 0; symbol < n; symbol++)
        (GZ_HUFF_COUNT[length[symbol]])++; // assumes lengths are within bounds
    
    /* No codes!*/
    if (GZ_HUFF_COUNT[0] == n)
        return 0;                   /* complete, but _gz_decode() will fail */
    
    /* Check for an over-subscribed or incomplete set of lengths */
    left = 1;                           /* one possible code of zero length */
    for (len = 1; len <= GZ_MAXBITS; len++) {
        left <<= 1;                     /* one more bit, double codes left */
        left -= GZ_HUFF_COUNT[len];     /* deduct count from possible codes */
        if (left < 0) return left;      /* over-subscribed--return negative */
    }                                   /* left > 0 means incomplete */
    
    /* generate offsets into symbol table for each length for sorting */
    offs[1] = 0;
    for (len = 1; len < GZ_MAXBITS; len++)
        offs[len + 1] = offs[len] + GZ_HUFF_COUNT[len];
    
    /* put symbols in table sorted by length, by symbol order within each
     * length
     */
    for (symbol = 0; symbol < n; symbol++)
        if (length[symbol] != 0)
            GZ_HUFF_SYMBOL[offs[length[symbol]]++] = symbol;
    
    /* return zero for complete set, positive for incomplete set */
    return left;
}

/* represents two huffman struct { short *count; short *symbol; } */
#define GZ_HUFF_LEN_COUNT       huff_len_count
#define GZ_HUFF_LEN_SYMBOL      huff_len_symbol
#define GZ_HUFF_LEN_DEF         GZ_HUFF_LEN_COUNT[], GZ_HUFF_LEN_SYMBOL[]

#define GZ_HUFF_DIST_COUNT      huff_dist_count
#define GZ_HUFF_DIST_SYMBOL     huff_dist_symbol
#define GZ_HUFF_DIST_DEF        GZ_HUFF_DIST_COUNT[], GZ_HUFF_DIST_SYMBOL[]

stock _gz_codes(GZ_STATE_DEF, GZ_HUFF_LEN_DEF, GZ_HUFF_DIST_DEF)
{LogToGame("codes");////////////////////////////////////
    decl symbol;        /* decoded symbol */
    decl len;           /* length for copy */
    decl dist;          /* distance for copy */
    static lens[29] = { /* Size base for length codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
    static lext[29] = { /* Extra bits for length codes 257..285 */
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    static dists[30] = { /* Offset base for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577 };
    static dext[30] = { /* Extra bits for distance codes 0..29 */
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13 };
    
    /* decode literals and length/distance pairs */
    do {
        symbol = _gz_decode(GZ_STATE, GZ_HUFF_LEN_COUNT, GZ_HUFF_LEN_SYMBOL);
        if (symbol == -1 && GZ_STATE[GZ_STATE_ERROR])
            return -1;
        if (symbol < 0) return symbol;      /* invalid symbol */
        if (symbol < 256) {                 /* literal: symbol is the byte */
            /* write out the literal */
            decl b[1];
            b[0] = symbol;
            LogToGame("write('%c')", symbol);///////////////////////////
            if (!WriteFile(Handle:GZ_STATE[GZ_STATE_OUT], b, 1, 1)) {
                GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_OUTPUT_FILE_BAD;
                return -1;
            }
        }
        else if (symbol > 256) {            /* length */
            /* get and compute length */
            symbol -= 257;
            if (symbol >= 29) {             /* invalid fixed code */
                GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INVALID_FIXED_CODE;
                return -1;
            }
            
            decl bits;
            _GZ_BITS(bits,lext[symbol]);
            len = lens[symbol] + bits;
            
            /* get and check distance */
            symbol = _gz_decode(GZ_STATE, GZ_HUFF_DIST_COUNT,
                                GZ_HUFF_DIST_SYMBOL);
            if (symbol == -1 && GZ_STATE[GZ_STATE_ERROR])
                return -1;
            if (symbol < 0) return symbol;  /* invalid symbol */
            _GZ_BITS(bits, dext[symbol]);
            dist = dists[symbol] + bits;
            
            /* copy length bytes from distance bytes back */
            for (new i=0; i<len; i++) {
                decl byte[1];
                _GZ_BYTE_GET(byte[0],-dist + i,SEEK_CUR,true);
                LogToGame("write('%c')", byte[0]);///////////////////////////
                if (!WriteFile(Handle:GZ_STATE[GZ_STATE_OUT], byte, 1, 1)) {
                    GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_OUTPUT_FILE_BAD;
                    return -1;
                }
            }
        }
    } while (symbol != 256);
    
    /* done with a valid fixed or dynamic block */
    return 0;
}

stock _gz_stored(GZ_STATE_DEF)
{LogToGame("stored");////////////////////////////////////
    decl len;   /* Length of stored block */

    /* Discard leftover bits from current byte (assumes BITCNT < 8) */
    GZ_STATE[GZ_STATE_BITBUF] = 0;
    GZ_STATE[GZ_STATE_BITCNT] = 0;

    /* Get length */
    _GZ_BYTE(len);
    decl _len;
    _GZ_BYTE(_len);
    len |= _len << 8;

    /* Check against its one's complement */
    decl _byte1, _byte2;
    _GZ_BYTE(_byte1);
    _GZ_BYTE(_byte2);
    if (_byte1 != (~len & 0xff) || _byte2 != (~_len & 0xff))
    {
        /* Didn't match complement! */
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_BAD_BLOCK;
        return -1;
    }

    /* Copy `len` bytes from input to output */
    decl buffer[len];
    decl ret;
    if ((ret = ReadFile(Handle:GZ_STATE[GZ_STATE_IN], buffer, len, 1)) != len)
    {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INPUT_FILE_BAD;
        if (ret != -1)
            /* Not enough input */
            GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
        return -1;
    }
    if (!WriteFile(Handle:GZ_STATE[GZ_STATE_OUT], buffer, len, 1))
    {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_OUTPUT_FILE_BAD;
        return -1;
    }

    return 0;
}

stock _gz_fixed(GZ_STATE_DEF)
{LogToGame("fixed");////////////////////////////////////
    static virgin = 1;
    static lencnt[GZ_MAXBITS + 1], lensym[GZ_FIXLCODES];
    static distcnt[GZ_MAXBITS + 1], distsym[GZ_MAXDCODES];
    
    if (virgin) {
        decl symbol;
        decl lengths[GZ_FIXLCODES];
        
        /* literal/length table */
        for (symbol = 0; symbol < 144; symbol++)
            lengths[symbol] = 8;
        for (; symbol < 256; symbol++)
            lengths[symbol] = 9;
        for (; symbol < 280; symbol++)
            lengths[symbol] = 7;
        for (; symbol < GZ_FIXLCODES; symbol++)
            lengths[symbol] = 8;
        if (_gz_construct(lencnt, lensym, lengths, GZ_FIXLCODES) == -1
            && GZ_STATE[GZ_STATE_ERROR])
            return -1;
        
        /* distance table */
        for (symbol = 0; symbol < GZ_MAXDCODES; symbol++)
            lengths[symbol] = 5;
        if (_gz_construct(distcnt, distsym, lengths, GZ_MAXDCODES) == -1
            && GZ_STATE[GZ_STATE_ERROR])
            return -1;
        
        /* do this just once */
        virgin = 0;
    }
    
    /* decode data until end-of-block code */
    return _gz_codes(GZ_STATE, lencnt, lensym, distcnt, distsym);
}

stock _gz_dynamic(GZ_STATE_DEF)
{LogToGame("dynamic");////////////////////////////////////
    decl bits;                          /* buffer for _GZ_BITS calls */
    decl nlen, ndist, ncode;            /* number of lengths in descriptor */
    decl index;                         /* index of lengths[] */
    decl err;                           /* construct() return value */
    decl lengths[GZ_MAXCODES];          /* descriptor code lengths */
    decl lencnt[GZ_MAXBITS+1], lensym[GZ_MAXLCODES];    /* lencode memory */
    decl distcnt[GZ_MAXBITS+1], distsym[GZ_MAXDCODES];  /* distcode memory */
    decl order[19] =                    /* permutation of code length codes */
        { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    
    /* get number of lengths in each table */
    _GZ_BITS(bits,5);
    nlen = bits + 257;
    
    _GZ_BITS(bits,5);
    ndist = bits + 1;
    
    _GZ_BITS(bits,4);
    ncode = bits + 4;
    
    /* check lengths */
    if (nlen > GZ_MAXLCODES || ndist > GZ_MAXDCODES) {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_HUFF_BAD_COUNTS;
        return -1;
    }
    
    /* read code length code lengths (really), missing lengths are zero */
    for (index = 0; index < ncode; index++) {
        _GZ_BITS(bits,3);
        lengths[order[index]] = bits;
    }
    for (; index < 19; index++)
        lengths[order[index]] = 0;
    
    /* build huffman table for code lengths codes (use lencnt/sym
     * temporarily)
     */
    err = _gz_construct(lencnt, lensym, lengths, 19);
    if (err == -1 && GZ_STATE[GZ_STATE_ERROR])
        return -1;
    else if (err != 0) {
        /* require complete code set here */
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_REQUIRE_COMPLETE_SET;
        return -1;
    }
    
    /* read length/literal and distance code length tables */
    index = 0;
    while (index < nlen + ndist) {
        decl symbol;            /* decoded value */
        decl len;               /* last length to repeat */
        
        symbol = _gz_decode(GZ_STATE, lencnt, lensym);
        if (symbol == -1 && GZ_STATE[GZ_STATE_ERROR])
            return -1;
        if (symbol < 16)           /* length in 0..15 */
            lengths[index++] = symbol;
        else {                          /* repeat instruction */
            len = 0;                    /* assume repeating zeros */
            if (symbol == 16) {         /* repeat last length 3..6 times */
                if (index == 0) {       /* no last length! */
                    GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_NO_LAST_LENGTH;
                    return -1;
                }
                len = lengths[index - 1];       /* last length */
                
                _GZ_BITS(bits,2);
                symbol = 3 + bits;
            }
            else if (symbol == 17) {    /* repeat zero 3..10 times */
                _GZ_BITS(bits,3);
                symbol = 3 + bits;
            }
            else {                      /* == 18, repeat zero 11..138 times */
                _GZ_BITS(bits,7);
                symbol = 11 + bits;
            }
            
            if (index + symbol > nlen + ndist) {
                GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_TOO_MANY_LENGTHS;
                return -1;              /* too many lengths! */
            }
            
            while (symbol--)            /* repeat last or zero symbol times */
                lengths[index++] = len;
        }
    }    
    
    /* check for end-of-block code -- there better be one! */
    if (lengths[256] == 0) {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_NO_END_OF_BLOCK_CODE;
        return -1;
    }

    /* build huffman table for literal/length codes */
    err = _gz_construct(lencnt, lensym, lengths, nlen);
    if (err == -1 && GZ_STATE[GZ_STATE_ERROR])
        return -1;
    if (err < 0 || (err > 0 && nlen - lencnt[0] != 1)) {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INCOMPLETE_ONLY_ONE;
        return -1;      /* only allow incomplete codes if just one code */
    }

    /* build huffman table for distance codes */
    err = _gz_construct(distcnt, distsym, lengths[nlen], ndist);
    if (err == -1 && GZ_STATE[GZ_STATE_ERROR])
        return -1;
    if (err < 0 || (err > 0 && ndist - distcnt[0] != 1)) {
        GZ_STATE[GZ_STATE_ERROR] = GZ_ERROR_INCOMPLETE_ONLY_ONE;
        return -1;      /* only allow incomplete codes if just one code */
    }
    
    /* decode data until end-of-block code */
    return _gz_codes(GZ_STATE, lencnt, lensym, distcnt, distsym);
}
