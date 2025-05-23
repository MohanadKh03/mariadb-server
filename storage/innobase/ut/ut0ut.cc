/*****************************************************************************

Copyright (c) 1994, 2017, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2017, 2022, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/***************************************************************//**
@file ut/ut0ut.cc
Various utilities for Innobase.

Created 5/11/1994 Heikki Tuuri
********************************************************************/

#include "ha_prototypes.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef UNIV_INNOCHECKSUM
#include <mysql_com.h>
#include "ut0ut.h"
#include "trx0trx.h"
#include <string>
#include "log.h"
#include "my_cpu.h"
#ifndef DBUG_OFF
#include "rem0rec.h"
#endif

/**********************************************************//**
Returns the number of milliseconds since some epoch.  The
value may wrap around.  It should only be used for heuristic
purposes.
@return ms since epoch */
ulint
ut_time_ms(void)
/*============*/
{
	return static_cast<ulint>(my_interval_timer() / 1000000);
}
#endif /* !UNIV_INNOCHECKSUM */

/**********************************************************//**
Prints a timestamp to a file. */
void
ut_print_timestamp(
/*===============*/
	FILE*  file) /*!< in: file where to print */
{
#ifdef _WIN32
	SYSTEMTIME cal_tm;
	GetLocalTime(&cal_tm);
#else
	time_t	   tm;
	struct tm  cal_tm;
	time(&tm);
	localtime_r(&tm, &cal_tm);
#endif
	fprintf(file,
		IF_WIN("%u-%02u-%02u %02u:%02u:%02u %#zx",
		       "%d-%02d-%02d %02d:%02d:%02d %#zx"),
#ifdef _WIN32
		cal_tm.wYear,
		cal_tm.wMonth,
		cal_tm.wDay,
		cal_tm.wHour,
		cal_tm.wMinute,
		cal_tm.wSecond,
#else
		cal_tm.tm_year + 1900,
		cal_tm.tm_mon + 1,
		cal_tm.tm_mday,
		cal_tm.tm_hour,
		cal_tm.tm_min,
		cal_tm.tm_sec,
#endif
#ifdef UNIV_INNOCHECKSUM
		ulint{0}
#else
		ulint(pthread_self())
#endif
		);
}

#ifndef UNIV_INNOCHECKSUM

/**********************************************************//**
Sprintfs a timestamp to a buffer, 13..14 chars plus terminating NUL. */
void
ut_sprintf_timestamp(
/*=================*/
	char*	buf) /*!< in: buffer where to sprintf */
{
#ifdef _WIN32
	SYSTEMTIME cal_tm;
	GetLocalTime(&cal_tm);

	sprintf(buf, "%02u%02u%02u %2u:%02u:%02u",
		cal_tm.wYear % 100,
		cal_tm.wMonth,
		cal_tm.wDay,
		cal_tm.wHour,
		cal_tm.wMinute,
		cal_tm.wSecond);
#else
	time_t	   tm;
	struct tm  cal_tm;
	time(&tm);
	localtime_r(&tm, &cal_tm);
	sprintf(buf, "%02d%02d%02d %2d:%02d:%02d",
		cal_tm.tm_year % 100,
		cal_tm.tm_mon + 1,
		cal_tm.tm_mday,
		cal_tm.tm_hour,
		cal_tm.tm_min,
		cal_tm.tm_sec);
#endif
}

/*************************************************************//**
Prints the contents of a memory buffer in hex and ascii. */
void
ut_print_buf(
/*=========*/
	FILE*		file,	/*!< in: file where to print */
	const void*	buf,	/*!< in: memory buffer */
	ulint		len)	/*!< in: length of the buffer */
{
	const byte*	data;
	ulint		i;

	fprintf(file, " len " ULINTPF "; hex ", len);

	for (data = (const byte*) buf, i = 0; i < len; i++) {
		fprintf(file, "%02x", *data++);
	}

	fputs("; asc ", file);

	data = (const byte*) buf;

	for (i = 0; i < len; i++) {
		int	c = (int) *data++;
		putc(isprint(c) ? c : ' ', file);
	}

	putc(';', file);
}

/*************************************************************//**
Prints the contents of a memory buffer in hex. */
void
ut_print_buf_hex(
/*=============*/
	std::ostream&	o,	/*!< in/out: output stream */
	const void*	buf,	/*!< in: memory buffer */
	ulint		len)	/*!< in: length of the buffer */
{
	const byte*		data;
	ulint			i;

	static const char	hexdigit[16] = {
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
	};

	o << "(0x";

	for (data = static_cast<const byte*>(buf), i = 0; i < len; i++) {
		byte	b = *data++;
		o << hexdigit[int(b) >> 4] << hexdigit[b & 15];
	}

	o << ")";
}

/*************************************************************//**
Prints the contents of a memory buffer in hex and ascii. */
void
ut_print_buf(
/*=========*/
	std::ostream&	o,	/*!< in/out: output stream */
	const void*	buf,	/*!< in: memory buffer */
	ulint		len)	/*!< in: length of the buffer */
{
	const byte*	data;
	ulint		i;

	for (data = static_cast<const byte*>(buf), i = 0; i < len; i++) {
		int	c = static_cast<int>(*data++);
		o << (isprint(c) ? static_cast<char>(c) : ' ');
	}

	ut_print_buf_hex(o, buf, len);
}

/** Get a fixed-length string, quoted as an SQL identifier.
If the string contains a slash '/', the string will be
output as two identifiers separated by a period (.),
as in SQL database_name.identifier.
 @param		[in]	trx		transaction (NULL=no quotes).
 @param		[in]	name		table name.
 @retval	String quoted as an SQL identifier.
*/
std::string
ut_get_name(
	const trx_t*	trx,
	const char*	name)
{
	/* 2 * NAME_LEN for database and table name,
	and some slack for the #mysql50# prefix and quotes */
	char		buf[3 * NAME_LEN];
	const char*	bufend;

	bufend = innobase_convert_name(buf, sizeof buf,
				       name, strlen(name),
				       trx ? trx->mysql_thd : NULL);
	buf[bufend - buf] = '\0';
	return(std::string(buf, 0, size_t(bufend - buf)));
}

/**********************************************************************//**
Outputs a fixed-length string, quoted as an SQL identifier.
If the string contains a slash '/', the string will be
output as two identifiers separated by a period (.),
as in SQL database_name.identifier. */
void
ut_print_name(
/*==========*/
	FILE*		f,	/*!< in: output stream */
	const trx_t*	trx,	/*!< in: transaction */
	const char*	name)	/*!< in: name to print */
{
	/* 2 * NAME_LEN for database and table name,
	and some slack for the #mysql50# prefix and quotes */
	char		buf[3 * NAME_LEN];
	const char*	bufend;

	bufend = innobase_convert_name(buf, sizeof buf,
				       name, strlen(name),
				       trx ? trx->mysql_thd : NULL);

	if (fwrite(buf, 1, size_t(bufend - buf), f) != size_t(bufend - buf)) {
		perror("fwrite");
	}
}

/**********************************************************************//**
Catenate files. */
void
ut_copy_file(
/*=========*/
	FILE*	dest,	/*!< in: output file */
	FILE*	src)	/*!< in: input file to be appended to output */
{
	long	len = ftell(src);
	char	buf[4096];

	rewind(src);
	do {
		size_t	maxs = len < (long) sizeof buf
			? (size_t) len
			: sizeof buf;
		size_t	size = fread(buf, 1, maxs, src);
		if (fwrite(buf, 1, size, dest) != size) {
			perror("fwrite");
		}
		len -= (long) size;
		if (size < maxs) {
			break;
		}
	} while (len > 0);
}

/** Convert an error number to a human readable text message.
The returned string is static and should not be freed or modified.
@param[in]	num	InnoDB internal error number
@return string, describing the error */
const char*
ut_strerr(
	dberr_t	num)
{
	switch (num) {
	case DB_SUCCESS:
		return("Success");
	case DB_SUCCESS_LOCKED_REC:
		return("Success, record lock created");
	case DB_ERROR:
		return("Generic error");
	case DB_READ_ONLY:
		return("Read only transaction");
	case DB_INTERRUPTED:
		return("Operation interrupted");
	case DB_OUT_OF_MEMORY:
		return("Cannot allocate memory");
	case DB_OUT_OF_FILE_SPACE:
		return("Out of disk space");
	case DB_LOCK_WAIT:
		return("Lock wait");
	case DB_DEADLOCK:
		return("Deadlock");
	case DB_RECORD_CHANGED:
		return("Record changed");
#ifdef WITH_WSREP
	case DB_ROLLBACK:
		return("Rollback");
#endif
	case DB_DUPLICATE_KEY:
		return("Duplicate key");
	case DB_MISSING_HISTORY:
		return("Required history data has been deleted");
	case DB_TABLE_NOT_FOUND:
		return("Table not found");
	case DB_TOO_BIG_RECORD:
		return("Record too big");
	case DB_TOO_BIG_INDEX_COL:
		return("Index columns size too big");
	case DB_LOCK_WAIT_TIMEOUT:
		return("Lock wait timeout");
	case DB_NO_REFERENCED_ROW:
		return("Referenced key value not found");
	case DB_ROW_IS_REFERENCED:
		return("Row is referenced");
	case DB_CANNOT_ADD_CONSTRAINT:
		return("Cannot add constraint");
	case DB_CORRUPTION:
		return("Data structure corruption");
	case DB_CANNOT_DROP_CONSTRAINT:
		return("Cannot drop constraint");
	case DB_TABLESPACE_EXISTS:
		return("Tablespace already exists");
	case DB_TABLESPACE_DELETED:
		return("Tablespace deleted or being deleted");
	case DB_TABLESPACE_NOT_FOUND:
		return("Tablespace not found");
	case DB_LOCK_TABLE_FULL:
		return("Lock structs have exhausted the buffer pool");
	case DB_FOREIGN_DUPLICATE_KEY:
		return("Foreign key activated with duplicate keys");
	case DB_FOREIGN_EXCEED_MAX_CASCADE:
		return("Foreign key cascade delete/update exceeds max depth");
	case DB_TOO_MANY_CONCURRENT_TRXS:
		return("Too many concurrent transactions");
	case DB_UNSUPPORTED:
		return("Unsupported");
	case DB_INVALID_NULL:
		return("NULL value encountered in NOT NULL column");
	case DB_STATS_DO_NOT_EXIST:
		return("Persistent statistics do not exist");
	case DB_FAIL:
		return("Failed, retry may succeed");
	case DB_OVERFLOW:
		return("Overflow");
	case DB_UNDERFLOW:
		return("Underflow");
	case DB_STRONG_FAIL:
		return("Failed, retry will not succeed");
	case DB_ZIP_OVERFLOW:
		return("Zip overflow");
	case DB_RECORD_NOT_FOUND:
		return("Record not found");
	case DB_CHILD_NO_INDEX:
		return("No index on referencing keys in referencing table");
	case DB_PARENT_NO_INDEX:
		return("No index on referenced keys in referenced table");
	case DB_FTS_INVALID_DOCID:
		return("FTS Doc ID cannot be zero");
	case DB_INDEX_CORRUPT:
		return("Index corrupted");
	case DB_UNDO_RECORD_TOO_BIG:
		return("Undo record too big");
	case DB_END_OF_INDEX:
		return("End of index");
	case DB_IO_ERROR:
		return("I/O error");
	case DB_NOT_FOUND:
		return("not found");
	case DB_ONLINE_LOG_TOO_BIG:
		return("Log size exceeded during online index creation");
	case DB_IDENTIFIER_TOO_LONG:
		return("Identifier name is too long");
	case DB_FTS_EXCEED_RESULT_CACHE_LIMIT:
		return("FTS query exceeds result cache limit");
	case DB_TEMP_FILE_WRITE_FAIL:
		return("Temp file write failure");
	case DB_CANT_CREATE_GEOMETRY_OBJECT:
		return("Can't create specified geometry data object");
	case DB_CANNOT_OPEN_FILE:
		return("Cannot open a file");
	case DB_TABLE_CORRUPT:
		return("Table is corrupted");
	case DB_FTS_TOO_MANY_WORDS_IN_PHRASE:
		return("Too many words in a FTS phrase or proximity search");
	case DB_DECRYPTION_FAILED:
		return("Table is compressed or encrypted but uncompress or decrypt failed.");
	case DB_IO_PARTIAL_FAILED:
		return("Partial IO failed");
	case DB_COMPUTE_VALUE_FAILED:
		return("Compute generated column failed");
	case DB_NO_FK_ON_S_BASE_COL:
		return("Cannot add foreign key on the base column "
		       "of stored column");
	case DB_IO_NO_PUNCH_HOLE:
		return ("File system does not support punch hole (trim) operation.");
	case DB_PAGE_CORRUPTED:
		return("Page read from tablespace is corrupted.");

	/* do not add default: in order to produce a warning if new code
	is added to the enum but not added here */
	}

	/* we abort here because if unknown error code is given, this could
	mean that memory corruption has happened and someone's error-code
	variable has been overwritten with bogus data */
	ut_error;

	/* NOT REACHED */
	return("Unknown error");
}

namespace ib {

std::ostream &operator<<(std::ostream &lhs, const bytes_iec &rhs)
{
  static const char *sizes[]= {"B", "KiB", "MiB", "GiB", "TiB", "PiB",
                              "EiB", "ZiB", "YiB"};
  size_t i= 0;
  double d= rhs.get_double();
  for (; d > 512.0 && i < array_elements(sizes); i++, d/= 1024.0);
  lhs.precision(3);
  lhs << std::fixed << d << sizes[i];
  return lhs;
}

ATTRIBUTE_COLD logger& logger::operator<<(dberr_t err)
{
  m_oss << ut_strerr(err);
  return *this;
}

info::~info()
{
	sql_print_information("InnoDB: %s", m_oss.str().c_str());
}

warn::~warn()
{
	sql_print_warning("InnoDB: %s", m_oss.str().c_str());
}

/** true if error::~error() was invoked, false otherwise */
bool error::logged;

error::~error()
{
	sql_print_error("InnoDB: %s", m_oss.str().c_str());
	logged = true;
}

#ifdef _MSC_VER
/* disable warning
  "ib::fatal::~fatal': destructor never returns, potential memory leak"
   on Windows.
*/
#pragma warning (push)
#pragma warning (disable : 4722)
#endif

ATTRIBUTE_NORETURN
fatal::~fatal()
{
	sql_print_error("[FATAL] InnoDB: %s", m_oss.str().c_str());
	abort();
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

error_or_warn::~error_or_warn()
{
	if (m_error) {
		sql_print_error("InnoDB: %s", m_oss.str().c_str());
	} else {
		sql_print_warning("InnoDB: %s", m_oss.str().c_str());
	}
}

fatal_or_error::~fatal_or_error()
{
	sql_print_error(m_fatal ? "[FATAL] InnoDB: %s" : "InnoDB: %s",
			m_oss.str().c_str());
	if (m_fatal) {
		abort();
	}
}

} // namespace ib

#ifndef DBUG_OFF
static char dbug_print_buf[1024];

const char * dbug_print_rec(const rec_t* rec, const rec_offs* offsets)
{
	rec_printer r(rec, offsets);
	strmake(dbug_print_buf, r.str().c_str(), sizeof(dbug_print_buf) - 1);
	return dbug_print_buf;
}

const char * dbug_print_rec(const rec_t* rec, ulint info, const rec_offs* offsets)
{
	rec_printer r(rec, info, offsets);
	strmake(dbug_print_buf, r.str().c_str(), sizeof(dbug_print_buf) - 1);
	return dbug_print_buf;
}

const char * dbug_print_rec(const dtuple_t* tuple)
{
	rec_printer r(tuple);
	strmake(dbug_print_buf, r.str().c_str(), sizeof(dbug_print_buf) - 1);
	return dbug_print_buf;
}

const char * dbug_print_rec(const dfield_t* field, ulint n)
{
	rec_printer r(field, n);
	strmake(dbug_print_buf, r.str().c_str(), sizeof(dbug_print_buf) - 1);
	return dbug_print_buf;
}

const char * dbug_print_rec(const rec_t* rec, dict_index_t* index)
{
	rec_offs	offsets_[REC_OFFS_NORMAL_SIZE];
	rec_offs*	offsets		= offsets_;
	rec_offs_init(offsets_);
	mem_heap_t*	tmp_heap	= NULL;
	offsets = rec_get_offsets(rec, index, offsets, index->n_core_fields,
				  ULINT_UNDEFINED, &tmp_heap);
	rec_printer r(rec, offsets);
	strmake(dbug_print_buf, r.str().c_str(), sizeof(dbug_print_buf) - 1);
	return dbug_print_buf;
}
#endif /* !DBUG_OFF */

#endif /* !UNIV_INNOCHECKSUM */
