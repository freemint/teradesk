#ifndef __WDLGEVNT_H__
#define __WDLGEVNT_H__

typedef _LONG fixed;

/* Mouse rectangle for EVNT_multi() */
#ifndef __MOBLK
#define __MOBLK
typedef struct
{
	_WORD	m_out;
	_WORD	m_x;
	_WORD	m_y;
	_WORD	m_w;
	_WORD	m_h;
} MOBLK;
#endif

#ifndef __EVNT
#define __EVNT
typedef struct
{
    _WORD    mwhich;
    _WORD    mx;
    _WORD    my;
    _WORD    mbutton;
    _WORD    kstate;
    _WORD    key;
    _WORD    mclicks;
    _WORD    reserved[9];
    _WORD    msg[16];
} EVNT;
#endif

/*	Maus-Position/Status und Tastatur-Status (evnt_button, evnt_multi)	*/
typedef struct
{
	_WORD x;
	_WORD y;
	_WORD bstate;
	_WORD kstate;
} EVNTDATA;

#endif /* __WDLGEVNT_H__ */
