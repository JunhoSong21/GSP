#include "NetworkHeader.h"
#include "EXP_OVER.h"

EXP_OVER::EXP_OVER()
{
	::ZeroMemory(&_over, sizeof(_over));
	_iotype = IOType::IO_ACCEPT;
	::ZeroMemory(_buf, BUF_SIZE);
	_wsa_buf.buf = _buf;
	_wsa_buf.len = BUF_SIZE;
}

EXP_OVER::EXP_OVER(IOType iotype) : _iotype(iotype)
{
	::ZeroMemory(&_over, sizeof(_over));
	::ZeroMemory(_buf, BUF_SIZE);
	_wsa_buf.buf = _buf;
	_wsa_buf.len = BUF_SIZE;
}
