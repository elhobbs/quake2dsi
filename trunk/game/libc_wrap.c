#define FUNCTION(x) int x(void) { }

FUNCTION(exit)
FUNCTION(_exit)
FUNCTION(_gettimeofday)
FUNCTION(_times)
FUNCTION(isatty)
FUNCTION(_sbrk)
FUNCTION(_getpid)
FUNCTION(_kill)
FUNCTION(_write)
FUNCTION(_close)
FUNCTION(_fstat)
FUNCTION(_lseek)
FUNCTION(_read)

int main(void)
{
}

void __wrap_printf(void)
{
}
