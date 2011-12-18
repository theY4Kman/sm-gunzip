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
//  Gzip header defines
// =============================================================================

#define GZ_ID1  0x1f    // Magic
#define GZ_ID2  0x8b    // Number

#define GZ_CM_DEFLATE   8   // The DEFLATE compression method

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

#define GZ_LAST_BLOCK       1

#define GZ_BLOCK_STORED     0
#define GZ_BLOCK_FIXED      1
#define GZ_BLOCK_DYNAMIC    2
#define GZ_BLOCK_RESERVED   3


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


// =============================================================================
//  Error codes (see GetErrorMessage for messages)
// =============================================================================

#define GZ_ERROR_SUCCESS                0000

// 1-999: gzip errors
#define GZ_ERROR_BAD_MEMBER_HEADER      0001
#define GZ_ERROR_UNIMPLEMENTED_CM       0002
#define GZ_ERROR_BAD_BLOCK              0003

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
            errcode = %1; \
            GetErrorMessage(%1, error, maxlength); \
            Format(error, maxlength, "%s in member %d", error, \
                   current_member); \
            CloseHandle(gzfp); \
            return -1; \
        }

        new _state[GZ_STATE_SIZE];
        _state[GZ_STATE_IN] = _:gzfp;
        _state[GZ_STATE_ERROR] = GZ_ERROR_SUCCESS;
        
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
            _state[GZ_STATE_OUT] = _:OpenFile(inc_fname, "wb");
        }
        else
            _state[GZ_STATE_OUT] = _:OpenFile(out_file, "wb");
        
        // Make sure we can write to the output file
        if (Handle:_state[GZ_STATE_OUT] == INVALID_HANDLE)
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
                return -1; \
            }
            
            new last_block = _gz_bits(_state, 1);
            if (last_block == -1 && _state[GZ_STATE_ERROR])
                __GZ_ERROR(_state[GZ_STATE_ERROR])
                        
            new block_type = _gz_bits(_state, 2);
            if (block_type == -1 && _state[GZ_STATE_ERROR])
                __GZ_ERROR(_state[GZ_STATE_ERROR])
            
            PrintToServer("last_block: %d    block_type: %d", last_block, block_type);
            
            decl ret;
            switch (block_type)
            {
                case GZ_BLOCK_STORED: ret = _gz_stored(_state);
                case GZ_BLOCK_FIXED: ret = _gz_fixed(_state);
                case GZ_BLOCK_DYNAMIC: ret = _gz_dynamic(_state);
                default: __GZ_ERROR(GZ_ERROR_BAD_BLOCK)
            }
            
            if (ret == -1)
                __GZ_ERROR(_state[GZ_STATE_ERROR])
            
            if (last_block)
                break;
            current_block++;
        }
        
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
    switch (errcode)
    {
        case GZ_ERROR_SUCCESS:
            __GZ_ERRCPY("Success");
        
        // Gzip errors
        case GZ_ERROR_BAD_MEMBER_HEADER:
            __GZ_ERRCPY("Bad member header");
        case GZ_ERROR_UNIMPLEMENTED_CM:
            __GZ_ERRCPY("Compression method not implemented");
        case GZ_ERROR_BAD_BLOCK:
            __GZ_ERRCPY("Bad compressed block");
        
        // Library errors
        case GZ_ERROR_INPUT_FILE_BAD:
            __GZ_ERRCPY("Could not open input file");
        case GZ_ERROR_OUTPUT_FILE_BAD:
            __GZ_ERRCPY("Could not open output file");
        case GZ_ERROR_NOT_IMPLEMENTED:
            __GZ_ERRCPY("Feature not implemented");
    }
    
    __GZ_ERRCPY("Not an error");
}


// =============================================================================
//  Library-use functions, forward compatability not guaranteed whatsoever
// =============================================================================

stock _gz_PrintMemberToServer(id1, id2, cm, flg, mtime, xfl, os, crc32, isize,
                           xlen, const String:fname[], const String:fcomment[],
                           crc16)
{
#if defined(DEBUG)
    return;
#endif
    
    PrintToServer("ID1: %02x", id1);
    PrintToServer("ID2: %02x", id2);
    
    PrintToServer("CM: %d%s", cm, (cm == GZ_CM_DEFLATE) ? " (DEFLATE)" : "");
    
#define     __GZ_FLG(%1) (flg & (1<<(%1)) >> (%1))
    PrintToServer("Flags:");
    PrintToServer("   FTEXT    - %d", __GZ_FLG(0));
    PrintToServer("   FHCRC    - %d", __GZ_FLG(1));
    PrintToServer("   FEXTRA   - %d", __GZ_FLG(2));
    PrintToServer("   FNAME    - %d", __GZ_FLG(3));
    PrintToServer("   FCOMMENT - %d", __GZ_FLG(4));
    PrintToServer("   reserved - %d", __GZ_FLG(5));
    PrintToServer("   reserved - %d", __GZ_FLG(6));
    PrintToServer("   reserved - %d", __GZ_FLG(7));
}

#define _GZ_BITS(%1,%2) \
    %1 = _gz_bits(_state, (%2)); \
    if (%1 == -1 && _state[GZ_STATE_ERROR]) \
        return -1

stock _gz_bits(_state[GZ_STATE_SIZE], need)
{
    decl val;
    
    // Load at least `need` bits into val
    val = _state[GZ_STATE_BITBUF];
    while (_state[GZ_STATE_BITCNT] < need)
    {
        decl byte[1];
        /* Load eight bits */
        if (ReadFile(Handle:_state[GZ_STATE_IN], byte, 1, 1) != 1)
        {
            /* Out of input */
            _state[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
            return -1;
        }
        
        val |= byte[0] << _state[GZ_STATE_BITCNT];
        _state[GZ_STATE_BITCNT] += 8;
    }
    
    /* Drop `need` bits and update buffer, always zero to seven bits left */
    _state[GZ_STATE_BITBUF] = val >> need;
    _state[GZ_STATE_BITCNT] -= need;
    
    /* Return `need` bits, zeroing the bits above that */
    return val & ((1 << need) - 1);
}

#define _GZ_BYTE(%1) \
    %1 = _gz_byte(_state, false); \
    if (%1 == -1 && _state[GZ_STATE_ERROR]) \
        return -1

stock _gz_byte(_state[GZ_STATE_SIZE], peek)
{
    decl buf[1];
    decl loc;
    
    if (peek)
        loc = FilePosition(Handle:_state[GZ_STATE_IN]);
    
    new ret = ReadFile(Handle:_state[GZ_STATE_IN], buf, 1, 1);
    if (ret != 1)
    {
        _state[GZ_STATE_ERROR] = GZ_ERROR_INPUT_FILE_BAD;
        if (ret != -1)
            _state[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
        return -1;
    }
    
    if (peek)
        FileSeek(Handle:_state[GZ_STATE_IN], loc, SEEK_SET);
    
    return buf[0];
}

stock _gz_stored(_state[GZ_STATE_SIZE])
{
    decl len;   /* Length of stored block */
    
    /* Discard leftover bits from current byte (assumes BITCNT < 8) */
    _state[GZ_STATE_BITBUF] = 0;
    _state[GZ_STATE_BITCNT] = 0;
    
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
        _state[GZ_STATE_ERROR] = GZ_ERROR_BAD_BLOCK;
        return -1;
    }
    
    /* Copy `len` bytes from input to output */
    decl buffer[len];
    decl ret;
    if ((ret = ReadFile(Handle:_state[GZ_STATE_IN], buffer, len, 1)) != len)
    {
        _state[GZ_STATE_ERROR] = GZ_ERROR_INPUT_FILE_BAD;
        if (ret != -1)
            /* Not enough input */
            _state[GZ_STATE_ERROR] = GZ_ERROR_END_OF_FILE;
        return -1;
    }
    if (!WriteFile(Handle:_state[GZ_STATE_OUT], buffer, len, 1))
    {
        _state[GZ_STATE_ERROR] = GZ_ERROR_OUTPUT_FILE_BAD;
        return -1;
    }
    
    return 0;
}

stock _gz_fixed(_state[GZ_STATE_SIZE])
{
    PrintToServer("fixed");
    _state[GZ_STATE_ERROR] = GZ_ERROR_NOT_IMPLEMENTED;
    return -1;
}

stock _gz_dynamic(_state[GZ_STATE_SIZE])
{
    PrintToServer("dynamic");
    _state[GZ_STATE_ERROR] = GZ_ERROR_NOT_IMPLEMENTED;
    return -1;
}
