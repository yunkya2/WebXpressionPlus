/* Tag.h */


static void TagBr(WORK *);
static void TagA(WORK *);
static void Tag_A(WORK *);
static void TagImg(WORK *);
static void TagFrameset(WORK *);
static void TagFrame(WORK *);
static void TagHr(WORK *);
static void TagP(WORK *);
static void Tag_P(WORK *);
static void TagH1(WORK *);
static void Tag_H1(WORK *);
#if	0
static void TagH2(WORK *);
static void Tag_H2(WORK *);
#endif
static void TagH3(WORK *);
static void Tag_H3(WORK *);
#if	0
static void TagH4(WORK *);
static void Tag_H4(WORK *);
static void TagH5(WORK *);
static void Tag_H5(WORK *);
static void TagH6(WORK *);
static void Tag_H6(WORK *);
#endif
static void TagTitle(WORK *);
static void Tag_Title(WORK *);
static void TagHead(WORK *);
static void Tag_Head(WORK *);
static void TagCenter(WORK *);
static void Tag_Center(WORK *);
static void TagScript(WORK *);
static void Tag_Script(WORK *);
static void TagStyle(WORK *);
static void Tag_Style(WORK *);
static void TagPre(WORK *);
static void Tag_Pre(WORK *);
static void TagOl(WORK *);
static void Tag_Ol(WORK *);
static void TagUl(WORK *);
static void Tag_Ul(WORK *);
static void TagLi(WORK *);
static void TagTable(WORK *);
static void Tag_Td(WORK *);
static void Tag_Tr(WORK *);
static void TagDt(WORK *);
static void TagDd(WORK *);
static void TagInput(WORK *);
static void TagBody(WORK *);
static void TagComment(WORK *);


typedef void (*func_tag) (struct _work *);
func_tag FuncTag[]=
{
	TagBr,
	TagA,
	Tag_A,			/* 閉じるタグは Tag_A のように表記しときます */
	TagImg,
	TagFrameset,
	TagFrame,
	TagHr,
	TagP,
	Tag_P,
	TagH1,
	Tag_H1,
	TagH1,	/* TagH2, */
	Tag_H1,	/* 	Tag_H2, */
	TagH3,
	Tag_H3,
	TagH3,	/* TagH4, */
	Tag_H3,	/* Tag_H4, */
	TagH3,	/* TagH5, */
	Tag_H3,	/* Tag_H5, */
	TagH3,	/* TagH6, */
	Tag_H3,	/* Tag_H6, */
	TagTitle,
	Tag_Title,
	TagHead,
	Tag_Head,
	TagCenter,
	Tag_Center,
	TagScript,
	Tag_Script,
	TagStyle,
	Tag_Style,
	TagPre,
	Tag_Pre,
	TagOl,
	Tag_Ol,
	TagUl,
	Tag_Ul,
	TagLi,
	TagTable,
	Tag_Td,
	Tag_Tr,
	TagDt,
	TagDd,
	TagInput,
	TagBody,
	TagComment,
	NULL,
};


unsigned char *tag_str[]=
{
	"BR",
	"A",
	"/A",
	"IMG",
	"FRAMESET",
	"FRAME",
	"HR",
	"P",
	"/P",
	"H1",
	"/H1",
	"H2",
	"/H2",
	"H3",
	"/H3",
	"H4",
	"/H4",
	"H5",
	"/H5",
	"H6",
	"/H6",
	"TITLE",
	"/TITLE",
	"HEAD",
	"/HEAD",
	"CENTER",
	"/CENTER",
	"SCRIPT",
	"/SCRIPT",
	"STYLE",
	"/STYLE",
	"PRE",
	"/PRE",
	"OL",
	"/OL",
	"UL",
	"/UL",
	"LI",
	"TABLE",
	"/TD",
	"/TR",
	"DT",
	"DD",
	"INPUT",
	"BODY",
	"!",
	NULL
};


enum {
	ATTR_A_HREF = 0,
	ATTR_A_NAME,
	ATTR_A_TYPE,
};

unsigned char *attr_a_str[]=
{
	"HREF",
	"NAME",
	"TYPE",
	NULL
};


enum {
	ATTR_IMG_SRC = 0,
	ATTR_IMG_ALT,
	ATTR_IMG_HEIGHT,
	ATTR_IMG_WIDTH
};

unsigned char *attr_img_str[]=
{
	"SRC",
	"ALT",
	"HEIGHT",
	"WIDTH",
	NULL
};


enum {
	ATTR_FRAME_SRC = 0,
	ATTR_FRAME_NAME
};

unsigned char *attr_frame_str[]=
{
	"SRC",
	"NAME",
	NULL
};


enum {
	ATTR_P_ALIGN = 0
};

unsigned char *attr_p_str[]=
{
	"ALIGN",
	NULL
};


enum {
	ATTR_INPUT_TYPE = 0,
	ATTR_INPUT_NAME,
	ATTR_INPUT_VALUE,
	ATTR_INPUT_CHECKED
};

unsigned char *attr_input_str[]=
{
	"TYPE",
	"NAME",
	"VALUE",
	"CHECKED",
	NULL
};


enum {
	ATTR_INPUT_TYPE_RADIO = 0,
	ATTR_INPUT_TYPE_CHECKBOX,
	ATTR_INPUT_TYPE_RESET,
	ATTR_INPUT_TYPE_SUBMIT
};

unsigned char *attr_input_type_str[]=
{
	"RADIO",
	"CHECKBOX",
	"RESET",
	"SUBMIT",
	NULL
};


enum {
	ATTR_BODY_TEXT = 0,
	ATTR_BODY_LINK,
	ATTR_BODY_ALINK,
	ATTR_BODY_VLINK,
	ATTR_BODY_BGCOLOR
};

unsigned char *attr_body_str[]=
{
	"TEXT",
	"LINK",
	"ALINK",
	"VLINK",
	"BGCOLOR",
	NULL
};



