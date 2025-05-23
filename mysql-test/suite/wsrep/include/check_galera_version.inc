# ==== Purpose ====
# Enable tests to check the galera library version.
#
# ==== Usage ====
# --let $galera_version=3.6
# source include/check_galera_lib_version.inc;
#
# Parameters:
#   $galera_version
#     Version of the galera library required by the test.
#

--disable_query_log

# Required Version

eval SET @GALERA_VERSION='$galera_version';
SELECT CAST(REGEXP_REPLACE(@GALERA_VERSION,'^(\\d+)\\.(\\d+).*','\\1') AS UNSIGNED) INTO @GALERA_MAJOR_VERSION;
SELECT CAST(REGEXP_REPLACE(@GALERA_VERSION,'^(\\d+)\\.(\\d+).*','\\2') AS UNSIGNED) INTO @GALERA_MINOR_VERSION;
SELECT CAST(REGEXP_REPLACE(@GALERA_VERSION,'^(\\d+)\\.(\\d+)\\.(\\d+).*','\\3') AS UNSIGNED) INTO @GALERA_RELEASE_VERSION;

# Actual
SELECT VARIABLE_VALUE INTO @ACTUAL_GALERA_VERSION FROM INFORMATION_SCHEMA.GLOBAL_STATUS WHERE VARIABLE_NAME LIKE 'wsrep_provider_version';
SELECT CAST(REGEXP_REPLACE(@ACTUAL_GALERA_VERSION,'^(\\d+)\\.(\\d+).*','\\1') AS UNSIGNED) INTO @ACTUAL_GALERA_MAJOR_VERSION;
SELECT CAST(REGEXP_REPLACE(@ACTUAL_GALERA_VERSION,'^[\\d\\.]*(\\d+)\\.\\d+.*','\\1') AS UNSIGNED) INTO @ACTUAL_GALERA_MINOR_VERSION;
SELECT CAST(REGEXP_REPLACE(@ACTUAL_GALERA_VERSION,'^[\\d\\.]*\\.(\\d+).*','\\1') AS UNSIGNED) INTO @ACTUAL_GALERA_RELEASE_VERSION;
--enable_query_log

# For testing
#SELECT @GALERA_MAJOR_VERSION;
#SELECT @GALERA_MINOR_VERSION;
#SELECT @GALERA_RELEASE_VERSION;
#SELECT @ACTUAL_GALERA_MAJOR_VERSION;
#SELECT @ACTUAL_GALERA_MINOR_VERSION;
#SELECT @ACTUAL_GALERA_RELEASE_VERSION;

if (!`SELECT (@ACTUAL_GALERA_MINOR_VERSION > @GALERA_MINOR_VERSION) OR
              (@ACTUAL_GALERA_MINOR_VERSION = @GALERA_MINOR_VERSION AND
	       @ACTUAL_GALERA_RELEASE_VERSION >= @GALERA_RELEASE_VERSION)
   `)
{
    let actual_galera_version=`select @ACTUAL_GALERA_VERSION`;
    skip needs galera >= $galera_version, got $actual_galera_version;
}

--echo # Correct Galera library found
