/* gifl.h */

/* オリジナルの struct.h（コメント by Mitsuky） */

typedef struct {
	char	*addr;
	char	*addr256;
	char	*tmpaddr;
	char	*endaddr;
	short	hispeed;
	char	*srcfile;
	long	srcsize;
	long	buffsize;
	long	buffreq;
	short	color;
	short	colum;
	short	line;
	short	scolum;
	short	sline;
	short	ecolum;
	short	eline;
	short	file;
	short	tpcolor;
	short	pal_buf[256];
	short	tmp[1024];
} GIFLOAD;
