void test_scalefloat();
void test_scalematrix();

#include <tinker/shell.h>

class CTester : public CShell
{
public:
	CTester(int argc, char** args) : CShell(argc, args) {};
};

int main(int argc, char** args)
{
	CTester c(argc, args);

	test_scalefloat();
	test_scalematrix();
}
