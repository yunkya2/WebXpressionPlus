/* Entity.h */

enum {
	ENTITY_LT = 0,
	ENTITY_GT,
	ENTITY_AMP,
	ENTITY_QUOT,
	ENTITY_NBSP
};


char *entity_str[]=
{
	"lt",
	"gt",
	"amp",
	"quot",
	"nbsp",
	NULL
};


char entity_char[]=
{
	'<',
	'>',
	'&',
	'"',
	' '
};

