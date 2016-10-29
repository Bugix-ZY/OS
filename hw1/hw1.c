//hw1.c
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/printk.h> 
#include <linux/kobject.h> 
#include <linux/sysfs.h>
#include <linux/init.h> 
#include <linux/fs.h> 
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang");

static struct kobject *hw1_object;

static int mask = 111;
static char *name1 = "swap_string";
static char *name2 = "calc";
static char *name3 = "sum_tree";


static int pos = 0;
static char mystring[4096] = {0};
static char eq[4096] = {0};
static int calc_result = 0;
static char sum[4096] = {0};



module_param(mask, int, S_IRUSR | S_IWUSR |S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(mask, "integer");
module_param(name1, charp, 0000);
MODULE_PARM_DESC(name1, "name1:swap_string");
module_param(name2, charp, 0000);
MODULE_PARM_DESC(name2, "name2:calc");
module_param(name3, charp, 0000);
MODULE_PARM_DESC(name3, "name3:sum_tree");

/*
    /sys/kernel/hw1/swap_string
*/
static ssize_t swap_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count){
    int i = 0;
    int length = 0;
	sscanf(buf, "%d %s", &pos, &mystring);

	//calculate the actual length
    for(i = 0; i < (sizeof mystring / sizeof(char)); i++){
   	    if(mystring[i] != '\0')
            length += 1;
    }
    printk(KERN_INFO "string length: %d",length);

    //swap
    char temp[length];
    for(i = 0; i < length - pos; i++){
    	temp[i] = mystring[i + pos];
    }

    for(i = 0; i < pos; i++){
    	temp[i + length - pos] = mystring[i];
    }

    //assign
    for(i = 0; i < length; i++){
    	mystring[i] = temp[i];
    }


    return count;
}


static ssize_t swap_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf, "%s\n", mystring);
}

static struct kobj_attribute swap_string_attribute = __ATTR(name1, 0660, swap_show, swap_store);

/*
    /sys/kernel/hw1/calc
*/
static ssize_t calc_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count){
	int i,j,k,temp;
	int ops_len, num_len;
	int length = 0;
	int flag = 0;


	for(i = 0; buf[i] != '\0'; i++)
		length += 1;

	for(i = 0; i < length; i++)
		eq[i] = buf[i];

	char *token[length], *delime = " ", *cur = eq;
	int token_len = 0;
	for(i = 0; token[i] = strsep(&cur,delime); i++){
		printk(KERN_INFO "%s\n",token[i]);
		token_len++;
	}

	num_len = token_len;
	ops_len = token_len;

	long int num[num_len];
	char ops[ops_len];

	i = 0;
	j = 0;
	// //ditinguish integers and opsarators
	for(k = 0; k < token_len; k++){
		// if:+ - * % /  else:numbers
		if(strcmp(token[k],"+") == 0){
			ops[j] = '+';
			j = j + 1;
		}else if(strcmp(token[k],"-") == 0){
			ops[j] = '-';
			j = j + 1;
		}else if(strcmp(token[k],"*") == 0){
			ops[j] = '*';
			j = j + 1;
		}else if(strcmp(token[k],"/") == 0){
			ops[j] = '/';
			j = j + 1;
		}else if(strcmp(token[k],"%") == 0){
			ops[j] = '%';
			j = j + 1;		
		}else{
			char *p = token[k];
			kstrtol(p,10,&num[i]);
			i = i + 1;
		}
	}
	num_len = i;
	ops_len = j;

	for(i = 0; i < num_len; i++){
		printk(KERN_INFO "%d: %ld",i,num[i]);
	}
	for(j = 0; j < ops_len; j++){
		printk(KERN_INFO "%d: %c",j,ops[j]);
	}

	//calculate
	int loop = ops_len;
	temp = 0;
	for(i = 0; i < loop; i++){
		if(ops[i] == '*' || ops[i] == '/' || ops[i] == '%'){
			// calculate 
			if(ops[i] == '*'){
				temp = num[i] * num[i + 1];
			}else if(ops[i] == '/'){
				temp = num[i] / num[i + 1];
			}else if(ops[i] == '%'){
				temp = num[i] % num[i + 1];	
			}
			// shift!
			for(j = i; j < num_len; j++){
				num[j] = num[j + 1];
			}
			num[i] = temp;

			for(j = i; j < ops_len; j++){
				ops[j] = ops[j + 1];
			}			
			num_len = num_len - 1;
			ops_len = ops_len - 1;

		}
	}

	// + -
	calc_result = num[0];
	for(i = 0; i < ops_len; i++){
		if(ops[i] == '+'){
			calc_result = calc_result + num[i + 1];
		}else if(ops[i] == '-'){
			calc_result = calc_result - num[i + 1];
		}
	}

	return count;
}

static ssize_t calc_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf,"%d\n",calc_result);
}

static struct kobj_attribute calc_attribute = __ATTR(name2, 0660, calc_show, calc_store);


/*
	/sys/kernel/hw1/sum_tree
*/
static ssize_t sum_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count){

	int i , j, k, p, r, flag;
	long int res;
	int buf_len = 0, num_len = 0;
	char *delim = " ";
	for(i = 0; buf[i] != '\0'; i++){
		buf_len++;
	}

	//sum = "\0";
	char tree[buf_len];
	char *token[buf_len];
	char *num_str[buf_len];
	int layer[buf_len];
	char *cur = tree;

	for(i = 0, j = 0; i < buf_len; i++){
		if(buf[i] != '(' && buf[i] != ')'){
			tree[j] = buf[i];
		}else{
			tree[j] = ' ';
		}
		j = j + 1;
	}

	for(i = 0; token[i] = strsep(&cur, delim); i++){
		printk(KERN_INFO "%s\n",token[i]);
	}


	// token_len = i - 1, j(0, i-1);
	// num_len = k, k(0,num_len-1)
	for(j = 0,k = 0; j < i - 1; j++){
		if(token[j][0] != '\0'){
			num_str[k] = token[j];
			k = k + 1;
		}
	}
	num_len = k;

	printk(KERN_INFO "num_str:\n");
	for(i = 0; i < num_len; i++){
		printk(KERN_INFO "%d %s\n",i, num_str);
	}
	long int numbers[num_len];
	long int result[num_len];

	for(i = 0; i < num_len; i++){
		char *p = num_str[i];
		kstrtol(p,10,&numbers[i]);
		printk(KERN_INFO "%ld ",numbers[i]);
	}
	
	printk(KERN_INFO "numbers:\n");
	for(i = 0; i < num_len; i++){
		printk(KERN_INFO "%d %ld\n",i, numbers[i]);
	}
	// compute nodes'layer
	for(i = 0,p = 0; i < buf_len; i++){
		k = 0;
		if(buf[i] != '(' && buf[i] != ')' && buf[i] != ' '){
			if(flag == 1)
				continue;
			for(j = 0; j < i; j++){
				if(buf[j] == '(')
					k = k + 1;
				else if(buf[j] == ')')
					k = k - 1;
			}
			layer[p] = k;
			p = p + 1;
			flag = 1;
		}else{
			flag = 0;
		}
	}

	// p -> the previous element of j
	// searched node => tag 0, other wise #layer
	char s[4096] = {0};
	for(i = 0; i < 4096; i++){
		s[i] = '\0';
		sum[i] = '\0';
	}
	for(i = 0, r = 0; i < num_len; i++){
		res = numbers[0];
		j = 1;
		p = 0;
		while(j < num_len){
			flag = 0;
			// skip all the nodes which already have been searched
			while(layer[j] == 0){
				j = j + 1;
			}
			//bound
			if (j >= num_len){
				flag = 1;
				break;
			}

			// if new node found
			if(layer[j] > layer[p]){
				res += numbers[j];
				p = j;
				j = j + 1;
			}

			// if: all subtree's routes end || arrive at the last route
			// else if: current route ends
			if((layer[p] > layer[j] && layer[j] != 0) || j >= num_len){
				for(k = p; layer[k] != layer[j]; k--){
					layer[k] = 0;
				}
				layer[k] = 0;
				break;
			}else if(layer[p] == layer[j]){
				layer[p] = 0;
				break;
			}
		}
		if(flag == 0){
			result[r] = res;
			char temp[10];
			if(j == num_len){
				sprintf(&temp, "%ld", res);
			}else {
				sprintf(&temp, "%ld, ", res);
			}
			strcat(s,temp);
			r = r + 1;
		}

	}

	for(i = 0; s[i] != '\0'; i++){
		sum[i] = s[i];
	}

	printk(KERN_INFO "result\n");
	for(i = 0; i < r; i++){
		printk(KERN_INFO "%ld ", result[i]);
	}


	return count;
}

static ssize_t sum_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf,"%s\n",sum);
}
static struct kobj_attribute sum_tree_attribute = __ATTR(name1, 0660, sum_show, sum_store);


static int __init myModule_init(void){
    int error = 0;

    printk(KERN_INFO "Module initialized successfully \n");

    //create sysfs entry: hw1
    hw1_object = kobject_create_and_add("hw1",kernel_kobj);
    if(!hw1_object)
    	return -ENOMEM;
    //create attributes
    swap_string_attribute.attr.name = name1;
    calc_attribute.attr.name = name2;
    sum_tree_attribute.attr.name = name3;
   
    if(mask == 111){
    	sysfs_create_file(hw1_object, &swap_string_attribute.attr);
    	sysfs_create_file(hw1_object, &calc_attribute.attr);
    	sysfs_create_file(hw1_object, &sum_tree_attribute.attr);
    }else if(mask == 110){
		sysfs_create_file(hw1_object, &swap_string_attribute.attr);
    	sysfs_create_file(hw1_object, &calc_attribute.attr);
    }else if(mask == 101){
		sysfs_create_file(hw1_object, &swap_string_attribute.attr);
    	sysfs_create_file(hw1_object, &sum_tree_attribute.attr);
    }else if(mask == 100){
		sysfs_create_file(hw1_object, &swap_string_attribute.attr);
    }else if(mask == 011){
		sysfs_create_file(hw1_object, &calc_attribute.attr);
    	sysfs_create_file(hw1_object, &sum_tree_attribute.attr);
    }else if(mask == 010){
		sysfs_create_file(hw1_object, &calc_attribute.attr);
    }else if(mask == 001){
       	sysfs_create_file(hw1_object, &sum_tree_attribute.attr); 	
    }else if(mask == 000){

    }


    return 0;
}

static void __exit myModule_exit(void){
	printk(KERN_INFO "Exit!\n");
	kobject_put(hw1_object);
}


module_init(myModule_init);
module_exit(myModule_exit);
