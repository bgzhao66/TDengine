# -*- coding: utf-8 -*-

# basic type
TAOS_DATA_TYPE  = [
    "INT", "BIGINT", "SMALLINT", "TINYINT", "INT UNSIGNED", "BIGINT UNSIGNED", "SMALLINT UNSIGNED", "TINYINT UNSIGNED",
    "FLOAT", "DOUBLE",
    "BOOL",
    "BINARY", "NCHAR", "VARCHAR",
    "TIMESTAMP",
    # "MEDIUMBLOB", "BLOB", # add in 3.x
    # "DECIMAL", "NUMERIC", # add in 3.x
    "JSON", # only for tag
]

TAOS_NUM_TYPE   = [
    "INT", "BIGINT", "SMALLINT", "TINYINT", "INT UNSIGNED", "BIGINT UNSIGNED", "SMALLINT UNSIGNED", "TINYINT UNSIGNED", "FLOAT", "DOUBLE",
    # "DECIMAL", "NUMERIC", # add in 3.x
]
TAOS_CHAR_TYPE  = [
    "BINARY", "NCHAR", "VARCHAR",
]
TAOS_BOOL_TYPE  = ["BOOL",]
TAOS_TS_TYPE    = ["TIMESTAMP",]
TAOS_BIN_TYPE   = [
    "MEDIUMBLOB", "BLOB", # add in 3.x
]

TAOS_TIME_INIT = ["b", "u", "a", "s", "m", "h", "d", "w", "n", "y"]
TAOS_PRECISION = ["ms", "us", "ns"]
PRECISION_DEFAULT = "ms"
PRECISION = PRECISION_DEFAULT

TAOS_KEYWORDS = [
    "ABORT",		"CREATE",		"IGNORE",		"NULL",			"STAR",
    "ACCOUNT",		"CTIME",		"IMMEDIATE",	"OF",			"STATE",
    "ACCOUNTS",		"DATABASE",		"IMPORT",		"OFFSET",		"STATEMENT",
    "ADD",			"DATABASES",	"IN",			"OR",			"STATE_WINDOW",
    "AFTER",		"DAYS",			"INITIALLY",	"ORDER",		"STORAGE",
    "ALL",			"DBS",			"INSERT",		"PARTITIONS",	"STREAM",
    "ALTER",		"DEFERRED",		"INSTEAD",		"PASS",			"STREAMS",
    "AND",			"DELIMITERS",	"INT",			"PLUS",			"STRING",
    "AS",			"DESC",			"INTEGER",		"PPS",			"SYNCDB",
    "ASC",			"DESCRIBE",		"INTERVAL",		"PRECISION",	"TABLE",
    "ATTACH",		"DETACH",		"INTO",			"PREV",			"TABLES",
    "BEFORE",		"DISTINCT", 	"IS",			"PRIVILEGE",	"TAG",
    "BEGIN",		"DIVIDE",		"ISNULL",		"QTIME",		"TAGS",
    "BETWEEN",		"DNODE",		"JOIN",			"QUERIES",		"TBNAME",
    "BIGINT",		"DNODES",		"KEEP",			"QUERY",		"TIMES",
    "BINARY",		"DOT",			"KEY",			"QUORUM",		"TIMESTAMP",
    "BITAND",		"DOUBLE",		"KILL",			"RAISE",		"TINYINT",
    "BITNOT",		"DROP",			"LE",			"REM",			"TOPIC",
    "BITOR",		"EACH",			"LIKE",			"REPLACE",		"TOPICS",
    "BLOCKS",		"END",			"LIMIT",		"REPLICA",		"TRIGGER",
    "BOOL",			"EQ",			"LINEAR",		"RESET",		"TSERIES",
    "BY",			"EXISTS",		"LOCAL",		"RESTRICT",		"UMINUS",
    "CACHE",		"EXPLAIN",		"LP",			"ROW",			"UNION",
    "CACHELAST",	"FAIL",			"LSHIFT",		"RP",			"UNSIGNED",
    "CASCADE",		"FILE",			"LT",			"RSHIFT",		"UPDATE",
    "CHANGE",		"FILL",			"MATCH",		"SCORES",		"UPLUS",
    "CLUSTER",		"FLOAT",		"MAXROWS",		"SELECT",		"USE",
    "COLON",		"FOR",			"MINROWS",		"SEMI",			"USER",
    "COLUMN",		"FROM",			"MINUS",		"SESSION",		"USERS",
    "COMMA",		"FSYNC",		"MNODES",		"SET",			"USING",
    "COMP",			"GE",			"MODIFY",		"SHOW",			"VALUES",
    "COMPACT",		"GLOB",			"MODULES",		"SLASH",		"VARIABLE",
    "CONCAT",		"GRANTS",		"NCHAR",		"SLIDING",		"VARIABLES",
    "CONFLICT",		"GROUP",		"NE",			"SLIMIT",		"VGROUPS",
    "CONNECTION",	"GT",			"NONE",			"SMALLINT",		"VIEW",
    "CONNECTIONS",	"HAVING",		"NOT",			"SOFFSET",		"VNODES",
    "CONNS",		"ID",			"NOTNULL",		"STABLE",		"WAL",
    "COPY",			"IF",			"NOW",			"STABLES",		"WHERE",
]

NUM_FUNC = [
    "ABS", "ACOS", "ASIN", "ATAN", "CEIL", "COS", "FLOOR", "LOG", "POW", "ROUND", "SIN", "SQRT", "TAN",
]

STR_FUNC = [
    "CHAR_LENGTH", "CONCAT", "CONCAT_WS", "LENGTH", "LOWER","LTRIM", "RTRIM", "SUBSTR", "UPPER",
]

CONVER_FUNC = ["CASR", "TO_ISO8601", "TO_JSON", "TP_UNIXTIMESTAMP"]

SELECT_FUNC = [
    "APERCENTILE", "BOTTOM", "FIRST", "INTERP", "LAST", "MAX", "MIN", "PERCENTILE", "TAIL", "TOP", "UNIQUE",
]

AGG_FUNC = [
    "AVG", "COUNT", "ELAPSED", "LEASTSQUARES", "MODE", "SPREAD", "STDDEV", "SUM", "HYPERLOGLOG", "HISTOGRAM",
]

TS_FUNC = [
    "CSUM", "DERIVATIVE", "DIFF", "IRATE", "MAVG", "SAMPLE", "STATECOUNT", "STATEDURATION", "TWA"
]

SYSINFO_FUCN = [
    "DATABASE", "CLIENT_VERSION", "SERVER_VERSION", "SERVER_STATUS", "CURRENT_USER", "USER"
]



# basic data type boundary
TINYINT_MAX             = 127
TINYINT_MIN             = -128

TINYINT_UN_MAX          = 255
TINYINT_UN_MIN          = 0

SMALLINT_MAX            = 32767
SMALLINT_MIN            = -32768

SMALLINT_UN_MAX         = 65535
SMALLINT_UN_MIN         = 0

INT_MAX                 = 2_147_483_647
INT_MIN                 = -2_147_483_648

INT_UN_MAX              = 4_294_967_295
INT_UN_MIN              = 0

BIGINT_MAX              = 9_223_372_036_854_775_807
BIGINT_MIN              = -9_223_372_036_854_775_808

BIGINT_UN_MAX           = 18_446_744_073_709_551_615
BIGINT_UN_MIN           = 0

FLOAT_MAX               = 3.40E+38
FLOAT_MIN               = -3.40E+38

DOUBLE_MAX              = 1.7E+308
DOUBLE_MIN              = -1.7E+308

# schema boundary
BINARY_LENGTH_MAX       = 16374
NCAHR_LENGTH_MAX       = 4093
DBNAME_LENGTH_MAX      = 64

STBNAME_LENGTH_MAX      = 192
STBNAME_LENGTH_MIN      = 1

TBNAME_LENGTH_MAX       = 192
TBNAME_LENGTH_MIN       = 1

CHILD_TBNAME_LENGTH_MAX = 192
CHILD_TBNAME_LENGTH_MIN = 1

TAG_NAME_LENGTH_MAX     = 64
TAG_NAME_LENGTH_MIN     = 1

COL_NAME_LENGTH_MAX     = 64
COL_NAME_LENGTH_MIN     = 1

TAG_COUNT_MAX           = 128
TAG_COUNT_MIN           = 1

COL_COUNT_MAX           = 4096
COL_COUNT_MIN           = 2

TAG_COL_COUNT_MAX       = 4096
TAG_COL_COUNT_MIN       = 3

MNODE_SHM_SIZE_MAX      = 2_147_483_647
MNODE_SHM_SIZE_MIN      = 6_292_480
MNODE_SHM_SIZE_DEFAULT  = 6_292_480

VNODE_SHM_SIZE_MAX      = 2_147_483_647
VNODE_SHM_SIZE_MIN      = 6_292_480
VNODE_SHM_SIZE_DEFAULT  = 31_458_304

# time_init
TIME_MS = 1
TIME_US = TIME_MS/1000
TIME_NS = TIME_US/1000

TIME_S = 1000 * TIME_MS
TIME_M = 60 * TIME_S
TIME_H = 60 * TIME_M
TIME_D = 24 * TIME_H
TIME_W = 7 * TIME_D
TIME_N = 30 * TIME_D
TIME_Y = 365 * TIME_D


# session parameters
INTERVAL_MIN = 1 * TIME_MS if PRECISION == PRECISION_DEFAULT else 1 * TIME_US


# streams and related agg-function
SMA_INDEX_FUNCTIONS = ["MIN", "MAX"]
ROLLUP_FUNCTIONS = ["AVG", "SUM", "MIN", "MAX", "LAST", "FIRST"]
BLOCK_FUNCTIONS = ["SUM", "MIN", "MAX"]
SMA_WATMARK_MAXDELAY_INIT = ['a', "s", "m"]
WATERMARK_MAX = 900000
WATERMARK_MIN = 0

MAX_DELAY_MAX = 900000
MAX_DELAY_MIN = 1