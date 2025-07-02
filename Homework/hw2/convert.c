#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void convert_fp32_to_hex(int sign, int *exp_fp32_bin, int *frac_fp32_bin, char *hex) {
    uint32_t result = 0;
    int temp[32]={0};
    temp[0]=sign;
    for(int i=0;i<8;i++){
        temp[i+1]=exp_fp32_bin[i];
    }
    for(int i=1;i<24;i++){
        temp[i+8]=frac_fp32_bin[i];
    }
    for(int i=0;i<32;i++){
        if(temp[i]==1){
            result+=1u<<(31-i);
        }
    }
    sprintf(hex, "%08x", result);
}
enum Type{normal, subnormal, inf, nan, zero, unknown};
#define Type2String(type) (type == normal ? "normal" : (type == subnormal ? "subnormal" : (type == inf ? "inf" : (type == nan ? "nan" : "zero"))))
#define low_limit -127
#define high_limit 128
void convert2bin(char *hex, int *bin){
    int i = 0;
    while(hex[i] != '\0'){
        if(hex[i] >= '0' && hex[i] <= '9'){
            int num = hex[i] - '0';
            for(int j = 3; j >= 0; j--){
                bin[i*4 + j] = num % 2;
                num = num >> 1;
            }
        }
        else if(hex[i] >= 'a' && hex[i] <= 'f'){
            int num = hex[i] - 'a' + 10;
            for(int j = 3; j >= 0; j--){
                bin[i*4 + j] = num % 2;
                num = num >> 1;
            }
        }
        i++;
    }
}
void convert2hex(int *bin_start,int *bin_end, char *hex){  // bin_end is not included
    int i = 0;
    while(bin_start != bin_end){
        int num = 0;
        for(int j = 0; j < 4; j++){
            num = num * 2 + bin_start[j];
        }
        if(num >= 0 && num <= 9){
            hex[i] = num + '0';
        }
        else if(num >= 10 && num <= 15){
            hex[i] = num - 10 + 'a';
        }
        bin_start += 4;
        i++;
    }
    hex[i] = '\0';
}
enum Type findType(char *exp, char *frac){
    if(strcmp(exp, "00000000") == 0 && strcmp(frac, "00000000") == 0){
        return zero;
    }
    else if(strcmp(exp, "00000000") == 0 && strcmp(frac, "00000000") != 0){
        return subnormal;
    }
    else if(strcmp(exp, "ffffffff") == 0 && strcmp(frac, "00000000") == 0){
        return inf;
    }
    else if(strcmp(exp, "ffffffff") == 0 && strcmp(frac, "00000000") != 0){
        return nan;
    }
    else{
        return normal;
    }
}
enum Type checkType_fp32(long long exp_after_offset){
    if(exp_after_offset<=-127)
        return zero;
    if(exp_after_offset>=128)
        return inf;
    return normal;
}
long long bin2int(int *bin, int start, int end){ // end is not included
    long long num = 0;
    for(int i = start; i < end; i++){
        num = num * 2 + bin[i];
    }
    return num;
}
long long ca25_exp2true(int *exp_ca25,int start, int end)  //ca25 is a 32 bits binary number
{
    long long exp_ca25_int = bin2int(exp_ca25, start, end);
    long long exp_true=exp_ca25_int-2147483647;  //2^31-1
    return exp_true;
}
long long calc_offset(long long exp_true){
    long long offset=0;
    if(exp_true<low_limit){
        offset=exp_true-low_limit;
    }
    else if(exp_true>high_limit){
        offset=exp_true-high_limit;
    }
    //printf("offset: %lld\n",offset);
    return -offset;
}
int rounding_away(int *result){
    int check=0;
    for(int i=23;i>=0;i--){
        if(result[i]==0){
            result[i]=1;
            check=1;
            break;
        }
        else{
            result[i]=0;
        }
    }
    //printf("results: ");
    //for(int i=0;i<24;i++){
    //    printf("%d",result[i]);
    //}
    //printf("\n");
    if(check==0){
        return 1;
    }
    return 0;
}
enum Type bin_bit_operation(int *frac_ca25, int offset, enum Type type, int *result){  //frac_ca25 is a 32 bits binary number include ont bit for the integer part
    //printf("frac_ca25: ");
    //for(int i=0;i<32;i++){
    //    printf("%d",frac_ca25[i]);
    //}
    //printf("\n");
    if(offset==0){
        int check=0;
        for(int i=31;i>23;i--){
            if(frac_ca25[i]==1){
                check=1;
                break;
            }
        }
        //printf("check: %d\n",check);
        //printf("result: ");
        for(int i=0;i<24;i++){
            result[i]=frac_ca25[i];
            //printf("%d",result[i]);
        }
        //printf("\n");
        if(check==0){
            return type;
        }
        else{
            return rounding_away(result)==1?unknown:type;
        }
    }
    offset+=1;
    if(offset>0){
        if(type==normal){
            int check=0;
            int checkpoint=32-offset;
            checkpoint=checkpoint>0?checkpoint:0;
            for(int i=31;i>=checkpoint;i--){
                if(frac_ca25[i]==1){
                    check=1;
                    break;
                }
            }
            int start_point=31-offset;
            int temp[32]={0};
            for(int i=31;i>=0;i--){
                if(start_point>=0){
                    temp[i]=frac_ca25[start_point];
                    start_point--;
                }
                else{
                    temp[i]=0;
                }
            }
            for(int i=0;i<24;i++){
                result[i]=temp[i];
            }
            for(int i=24;i<32;i++){
                if(temp[i]==1){
                    check=1;
                    break;
                }
            }
            /*
            printf("temp: ");
            for(int i=0;i<32;i++){
                printf("%d",temp[i]);
            }
            printf("\n");
            printf("result: ");
            for(int i=0;i<24;i++){
                printf("%d",result[i]);
            }
            printf("\n");*/
            if(check==0){
                return subnormal;
            }
            else{
                return rounding_away(result)==1?unknown:subnormal;
            }
        }
        else if(type==subnormal){
            int check=0;
            int checkpoint=32-offset;
            checkpoint=checkpoint>0?checkpoint:0;
            for(int i=31;i>=checkpoint;i--){
                if(frac_ca25[i]==1){
                    check=1;
                    break;
                }
            }
            int start_point=31-offset;
            int temp[32]={0};
            for(int i=31;i>=0;i--){
                if(start_point>=0){
                    temp[i]=frac_ca25[start_point];
                    start_point--;
                }
                else{
                    temp[i]=0;
                }
            }
            for(int i=0;i<24;i++){
                result[i]=temp[i];
            }
            for(int i=24;i<32;i++){
                if(temp[i]==1){
                    check=1;
                    break;
                }
            }
            /*
            printf("temp: ");
            for(int i=0;i<32;i++){
                printf("%d",temp[i]);
            }
            printf("\n");
            printf("result: ");
            for(int i=0;i<24;i++){
                printf("%d",result[i]);
            }
            printf("\n");*/
            if(check==0){
                return subnormal;
            }
            else{
                return rounding_away(result)==1?unknown:subnormal;
            }
        }
        else if(type==zero){
            for(int i=23;i>=0;i--){
                result[i]=0;
            }
            return zero;
        }
    }
    else if(offset<0){
        if(type==normal){
            for(int i=23;i>=1;i--){
                result[i]=0;
            }
            result[0]=1;
            return inf;
        }
        else if(type==inf){
            for(int i=23;i>=1;i--){
                result[i]=0;
            }
            result[0]=1;
            return inf;
        }
        else if(type==nan){
            for(int i=23;i>=1;i--){
                result[i]=frac_ca25[i];
            }
            result[0]=1;
            return nan;
        }
    }
    return unknown;
}
void convert_dec2bin(long long dec, int *bin){
    int i = 7;
    //printf("dec: ");
    //printf("%lld\n", dec);
    while(dec != 0){
        bin[i] = dec % 2;
        //printf("%d", bin[i]);
        dec = dec / 2;
        i--;
    }
}
void convert_exptrue_fp32(long long exp_true, int *exp_fp32){
    exp_true+=127;
    convert_dec2bin(exp_true,exp_fp32);
}
enum Type readd(int *bin,int exp_after_offset){
    for(int i=0;i<=23;i++){
        bin[i]=0;
    }
    if (exp_after_offset+1==-127)
        return zero;
    bin[0]=1;
    if(exp_after_offset+1==128)
        return inf;
    else
        return normal;
}
long long exp_after_offset(long long offset,long long exp_true){
    return exp_true+offset;
}
enum Type double_checkfp32(long long exp_final,int *fp32_frac, enum Type previous_type){
    if(previous_type!=normal){
        return previous_type;
    }
    if(exp_final<=-127){
        fp32_frac[0]=0;
        return subnormal;
    }
    if(exp_final>=128){
        
        for(int i=0;i<24;i++){
            fp32_frac[i]=0;
        }
        fp32_frac[0]=1;
        return inf;
    }
    return previous_type;
}
int main(void) {
    char input[100];
    while(fgets(input, 100, stdin) != NULL){
        char hex[17];
        for(int i = 0; i < 16; i++){
            hex[i] = input[i];
        }
        hex[16] = '\0';
        //printf("hex: %s\n", hex);
        //hex[strlen(hex)-1] = '\0';
        //printf("hex: %s\n", hex);
        int bin[65] = {0};  //the last is lsb
        convert2bin(hex, bin);
        //printf("bin: ");
        //for(int i = 0; i < 64; i++){
        //    printf("%d", bin[i]);
        //}
        //printf("\n");
        // find sign bit:
        int sign = bin[0]== 0 ? 0 : 1;   // 0: positive, 1: negative
        // find exponent:
        char exp[9];
        convert2hex(bin+1, bin+33, exp);
        // find fraction:
        char frac[9];
        convert2hex(bin+33, bin+65, frac);
        // find type:
        enum Type types_ca25 = findType(exp, frac);
        if(types_ca25 == zero || types_ca25 == subnormal){
            printf("ca25 S=%d E=%s M=0.%s %s\n", sign, exp, frac, Type2String(types_ca25));
            //printf("ca25 S=%d E=%s M=0.%s %s\n", sign, exp, "000002", Type2String(types_ca25));
        }
        else{
            printf("ca25 S=%d E=%s M=1.%s %s\n", sign, exp, frac, Type2String(types_ca25));
        }
        //printf("ca25 S=%d E=%s M=%s %s\n", sign, exp, frac, Type2String(types));
        //printf("sign: %d\n", sign);
        // printf("exp: %s\n", exp);
        // printf("frac: %s\n", frac);
        //convert_offset(fp32_bin, ca25_bin);
        //long long frac_int = bin2int(bin, 33, 65);
        //printf("frac_int: %lld\n", frac_int);
        long long exp_true=ca25_exp2true(bin,1,33);
        //printf("exp_true: %lld\n",exp_true);
        long long offset=calc_offset(exp_true);
        int ca25_frac[32];
        if(types_ca25==normal||types_ca25==inf||types_ca25==nan){
            ca25_frac[0]=1;
        }
        else{
            ca25_frac[0]=0;
        }
        for(int i = 1; i < 32; i++){
            ca25_frac[i] = bin[i+32];
        }
        int fp32_frac[25];
        if(exp_true==-127){
            int temp[32];
            int check=ca25_frac[31];
            for(int i=31;i>0;i--){
                temp[i]=ca25_frac[i-1];
            }
            temp[0]=0;
            for(int i=0;i<24;i++)
                fp32_frac[i]=temp[i];
            for(int i=24;i<32;i++){
                if(temp[i]==1){
                    check=1;
                    break;
                }
            }
            if(check==1){
                rounding_away(fp32_frac);
            }
            enum Type type_low;
            if(fp32_frac[0]==1){
                type_low=normal;
            }
            else{
                type_low=subnormal;
            }
            char frac_fp32[7];
            fp32_frac[24]=0;
            convert2hex(fp32_frac+1,fp32_frac+25,frac_fp32);
            printf("fp32 S=%d E=00 M=%d.%s %s\n", sign, fp32_frac[0],frac_fp32, Type2String(type_low));
            char fp32_hex[9];
            int exp_fp32_bin[8]={0};
            convert_fp32_to_hex(sign, exp_fp32_bin, fp32_frac, fp32_hex);
            printf("%s\n", fp32_hex);
            continue;
        }
        enum Type types_fp32=bin_bit_operation(ca25_frac,offset,types_ca25,fp32_frac);
        long long exp_final;
        if(types_fp32==unknown){
            exp_final=exp_after_offset(offset,exp_true);
            types_fp32=readd(fp32_frac,exp_final);
            exp_final++;
        }
        else{
            exp_final=exp_after_offset(offset,exp_true);
        }
        types_fp32=double_checkfp32(exp_final,fp32_frac,types_fp32);
        if(types_fp32==zero){
            exp_final=-127;
        }
        if(types_fp32==inf){
            exp_final=128;
        }
        //double check
        /*
        if(checkType_fp32(exp_final)==inf){
            types_fp32=inf;
            for(int i=1;i<24;i++){
                fp32_frac[i]=0;
            }
            fp32_frac[0]=1;
        }
        else if(checkType_fp32(exp_final)==zero){
            types_fp32=zero;
            for(int i=0;i<24;i++){
                fp32_frac[i]=0;
            }
        }*/
        int exp_fp32_bin[8]={0};
        convert_exptrue_fp32(exp_final,exp_fp32_bin);
        char exp_fp32[3];
        convert2hex(exp_fp32_bin,exp_fp32_bin+8,exp_fp32);
        char frac_fp32[7];
        fp32_frac[24]=0;
        convert2hex(fp32_frac+1,fp32_frac+25,frac_fp32);
        printf("fp32 S=%d E=%s M=%d.%s %s\n", sign, exp_fp32, fp32_frac[0],frac_fp32, Type2String(types_fp32));
        //printf("fp32 S=0 E=00 M=0.ffc20e subnormal\n");
        char fp32_hex[9];
        convert_fp32_to_hex(sign, exp_fp32_bin, fp32_frac, fp32_hex);
        //printf("fp32 hexadecimal representation: %s\n", fp32_hex);
        printf("%s\n", fp32_hex);
        //printf("00000001\n");
        
    }
    return 0; 
}
