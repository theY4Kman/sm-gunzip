/**
 * =============================================================================
 * Yak's gunzip
 *     Concommand-line utility.
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

#pragma semicolon 1

#include <sourcemod>
#include <gunzip>

#define DEBUG

public Plugin:myinfo =
{
    name = "Yak's Gunzip Utility",
    author = "theY4Kman",
    description = "Extract gzip-compressed files",
    version = "0.0.0.1-unreleased",
    url = "http://github.com/theY4Kman/sm-gunzip/"
};

public OnPluginStart()
{
    new errcode = 0;
    decl String:error[256];
    
    decl String:gzipped_file[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, gzipped_file, sizeof(gzipped_file), "data/gunzip.sp.gz");
    
    decl String:out_file[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, out_file, sizeof(out_file), "data/gunzip.sp");
    
    new bytes = ExtractGzipFile(gzipped_file, out_file, error, sizeof(error), errcode);
    if (bytes == -1)
        LogError("Unable to open gzip file: (%04d) %s", errcode, error);
}
