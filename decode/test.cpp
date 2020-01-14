#include <gtkmm.h>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define OP_ENCODE 0
#define OP_DECODE 1

using namespace std;

class B64Converter
{
	public:
		static size_t calcDecodeLength(char*);
		static int b64_op(char*, int, char*, int, int);
};

size_t B64Converter::calcDecodeLength(char* b64input) {
    size_t len = strlen((const char*)b64input),
        padding = 0;

    if (b64input[len-1] == '=' && b64input[len-2] == '=')
        padding = 2;
    else if (b64input[len-1] == '=')
        padding = 1;

    return (len*3)/4 - padding;
}

int B64Converter::b64_op(char* in, int in_len, char *out, int out_len, int op) 
{
    int ret = 0;
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    if (op == 0)
    {   
        ret = BIO_write(b64, in, in_len);
        BIO_flush(b64);
        if (ret > 0)
        {
            ret = BIO_read(bio, out, sizeof(out));
        }

    } else
    {
        out_len = (int)B64Converter::calcDecodeLength(in);
        ret = BIO_write(bio, in, in_len);
        BIO_flush(bio);
        if (ret)
        {
            ret = BIO_read(b64, out, out_len);
        }
    }
    BIO_free(b64);
    return ret;
}

int main(int argc, char **argv)
{
	char *in = argv[1];
	char out[1024];
	char dec[1024];

	for (int i=0; i < strlen(out); i++)
	{
		out[i] = 0x0;
		dec[i] = 0x0;
	}

	int p = 0;
	printf("%s\n", in);
	printf("%d\n", (int)strlen(in));

	for (int i=0; i < strlen(in); i++)
	{
		if (in[i] == '=')
		{
			if ((i+1 < strlen(in)) && (i+2 < strlen(in)))
			{
				char code[3];
				code[0] = in[i+1];
				code[1] = in[i+2];
				code[2] = 0x0;
				char a = (char)strtol(code, 0, 16);
				printf("%x\n", a);
				out[p] = a;
				p++;
				i += 2;
			}
			else
			{
				out[p] = in[i];
				p++;
			}
		}
		else
		{
			out[p] = in[i];
			p++;
		}
	}
	out[p] = 0x0;

	printf("out : %s\n", out);
	printf("out length : %d\n", (int)strlen(out));

	B64Converter::b64_op((char *)(out), (size_t)strlen(out), dec, 0, OP_DECODE);

	printf("decoded : %s\n", dec);

	Glib::ustring ret = Glib::ustring(dec);
	cout << "result : " << ret << endl;

	return 0;
}
