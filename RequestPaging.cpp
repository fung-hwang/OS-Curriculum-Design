// RequestPaging.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "stdlib.h"
#include "windows.h"
#include "string.h"
#include "time.h"

//页表项
struct pageTableEntry {
	int frame = -1;			//物理块号
	int state = 0;			//状态位（是否调入内存）
	int visitFrequency = 0;	//访问字段（访问次数）
	int change = 0;			//修改位（是否发生变化，需要重新写入磁盘）
};

int RAM_Length = 1048576;	//内存地址总行数,假定1MB
int pageLength = 1024;		//页长、块长,假定1KB
int frameNum = 1024;		//物理块数,假定1K块
int frame_used[1024];		//标记物理块是否被使用
int remainingFrame = 1024;	//剩余块数
double f_standard = 0.35;	//标准高位缺页率
int frame_increase = 5;		//标准增加块数（再分配到某进程的物理块数）
int frame_proportion = 4;	//初分配时物理块数量占页数的frame_proportion分之一
int initRAM_flag = 0;		//是否初始化内存

//函数声明
void menu();
void loca(int x, int y);
void writeToRAM(int instruction_page);
void writeToDisk(int last_recently_used);
void initRAM();
void createProcess();
void run(int physical_address);
void LRU(struct pageTableEntry *, int, int, int);
void print_pageTableEntry(struct pageTableEntry *pagetable, int instruction_page);
void over();
void checkRAM();
void uninitialized_RAM();
int get_free_frame();

//菜单
void menu() {
	while (1)
	{
		system("cls");      //清屏 
		loca(46, 6);printf("请求分页存储管理模拟系统");
		loca(46, 10);printf("|      1.初始化内存    |");
		loca(46, 12);printf("|      2.查看内存      |");
		loca(46, 14);printf("|      3.执行程序      |");
		loca(46, 16);printf("|      4.退出系统      |");
		loca(80, 26);printf("Designed by Zhou");
		loca(46, 20);printf("------------------------");
		loca(52, 22);
		printf("请选择操作：");
		int t;   //选择功能
		scanf_s("%d", &t);
		switch (t)
		{
		case 1: initRAM();; break;
		case 2: checkRAM(); break;
		case 3: createProcess();break;
		case 4: over(); break;
		default:break;
		}
	}
}

void createProcess() {

	uninitialized_RAM();
	void uninitialized_RAM();
	int logic_address_length;	//逻辑地址总长度
	while (1) {
		system("cls");
		loca(40, 8);
		int logic_address_length_max = 256 * pageLength < remainingFrame * frame_proportion * pageLength ? 
			256 * pageLength : remainingFrame * 2 * pageLength;
		printf("逻辑地址长度要求为 [ 1 , %d ]", logic_address_length_max);
		loca(40, 10);
		printf("请输入逻辑地址总长度：");
		loca(62, 10);
		scanf_s("%d", &logic_address_length);
		//判定一下逻辑地址大小不合格就重来
		//逻辑地址大小取决于页表长度和内存的剩余物理块数量
		if (logic_address_length > 0 && logic_address_length < 256 * pageLength 
			&& logic_address_length < remainingFrame * 2 * pageLength) {
			break;
		}
		else{
			loca(40, 15);printf("逻辑地址长度异常，请重新输入!");
			Sleep(1000);
		}
	}
	int page_count = logic_address_length / pageLength + 1;	//逻辑地址的页数
	int frame_count = page_count / frame_proportion + 1;		//按照页数的若干分之一分配物理块数
	if (frame_count > remainingFrame) {
		printf("内存不够了");
	}
	else   //内存够则开始执行程序
	{
		//剩余物理块数减少
		remainingFrame -= frame_count;
		//创建页表
		/*用固定长度的数组必然会有一些问题，用可变数组会比较好，但暂不解决*/
		struct pageTableEntry pageTable[256];
		struct pageTableEntry *pagetable= pageTable;
		loca(40, 12);
		printf("页数：%d，分配到物理块数：%d", page_count, frame_count);

		getchar(); getchar();

		LRU(pagetable, page_count, frame_count, logic_address_length);

		free(pagetable);
	}
}

void LRU(struct pageTableEntry *pagetable, int page_count, int frame_count, int logic_length) {
	system("cls");
	int loca_control = 4;	//控制光标的行数
	int missing_page = 0;	//缺页数
	double f = 1;	//缺页率
	int arr_length = frame_count;	//空物理块数量

	//需要一个数据结构记录访问页号的顺序
	int *arr;	//arr数组记录页面使用顺序，下标越大越是近期使用
	if ((arr = (int *)malloc(frame_count * sizeof(int))) == NULL)
	{
		printf("（实际）分配内存空间失败，程序退出！");
		exit(0);
	}

	srand(time(NULL));	//与时间有关的种子，确保每次随机的结果不同
	loca(20, 0);
	printf("页表（只包含在内存中的页表项）                                          明细");
	loca(1, 1);
	printf("--------------------------------------------------------------------------------------------------------------------");
	loca(1, 2);
	printf("页号       物理块号         状态位         访问字段         修改位           地址映射              具体操作");
	loca(1, 3);
	printf("-------------------------------------------------------------------------------------------------------------------- ");


	//假设需要访问xxx条逻辑地址，每次逻辑地址通过随机函数得到
	for (int i = 0; i < page_count; i++) {
		/*虽然因为局部性原理，逻辑地址总是抱团出现，但对模拟系统而言，随机产生逻辑地址能更好的展示各种情况*/
		int instruction = (int)(rand() % logic_length);	//随机下一条指令的逻辑地址
		int instruction_page = instruction / pageLength;	//逻辑地址对应的页号
		int instruction_shifting = instruction % pageLength;	//逻辑地址的页内偏移

		loca(84, loca_control); 
		printf("【第%d条地址访问】", i + 1);
		loca(76, loca_control + 1); 
		printf("逻辑地址：%d", instruction);
		loca(76, loca_control + 2); 
		printf("页    号：%d", instruction_page);
		loca(76, loca_control + 3); 
		printf("页内偏移：%d", instruction_shifting);


		//如果下一个使用的页号已经在内存中了
		if ((pagetable + instruction_page)->state == 1) {
			int record = 0;
			for (int i = 0; i < frame_count - arr_length; i++) {
				if (arr[i] == instruction_page) {
					record = i;	//记录instruction_page原来所在的数组下标
					break;
				}
			}
			if (record != frame_count - arr_length - 1) {
				for (int i = record + 1; i < frame_count - arr_length; i++) {
					arr[i - 1] = arr[i];
				}
				arr[frame_count - arr_length - 1] = instruction_page;
			}
			(pagetable + instruction_page)->visitFrequency++;

			loca(96, loca_control + 1); 
			printf("第%d页已在内存中", instruction_page);
			loca(76, loca_control + 4); 
			printf("物理块号：%d", (pagetable + instruction_page)->frame);
		}//if

		else //下一个页号不在内存中,需要调入页面
		{
			loca(96, loca_control + 1);
			writeToRAM(instruction_page);
			//如果需要换出页面
			if (arr_length == 0) {
				int last_recently_used = arr[0];	//取出最近最久未使用的页面
				for (int i = 1; i < frame_count; i++) {
					arr[i - 1] = arr[i];
				}
				//换出页面的页表调整
				(pagetable + last_recently_used)->state = 0;
				(pagetable + instruction_page)->frame = (pagetable + last_recently_used)->frame;
				(pagetable + last_recently_used)->frame = -1;
				(pagetable + last_recently_used)->visitFrequency = 0;
				//打印物理块号
				loca(76, loca_control + 4); 
				printf("物理块号：%d", (pagetable + instruction_page)->frame);
				//处理撤出内存的页面的回写
				loca(96, loca_control + 2);
				if ((pagetable + last_recently_used)->change == 1) {
					writeToDisk(last_recently_used);	//页面修改过，需要写回
					(pagetable + last_recently_used)->change = 0;
				}
				else {
					printf("第%d页未修改，无需写回", last_recently_used);
				}
					
				//换入页面的页表调整
				(pagetable + instruction_page)->state = 1;	//调入的页面需改变状态
				(pagetable + instruction_page)->visitFrequency++;

				arr[frame_count - 1] = instruction_page;

				//实现可变分配局部置换，在缺页率过高时多分配一些物理块
				missing_page+=1;
				f = ((double)missing_page) / (i+1);	//计算缺页率
				loca(96, loca_control + 3);
				printf("缺页数：%d，缺页率%.2lf \n", missing_page,f);
				
				//如果缺页率>标准缺页率 且 内存有可以增加的位置，就增加物理块分配
				if (f > f_standard && remainingFrame>=f_standard) { 
					loca(96, loca_control + 4);
					printf("增加%d块物理块",frame_increase);
					arr_length += frame_increase;
					frame_count += frame_increase;
				
					//更新arr数组，使其可以容纳增加的物理块
					int * p= (int *) malloc(frame_count * sizeof(int));

					if ( p  == NULL )
					{
						printf("（实际）分配内存空间失败");
						exit(0);
					}
					for (int j = 0; j < frame_count - arr_length ; j++)
					{
						p[j] = arr[j];
					}

					arr = p;
					//free(p);
				}//if
				
				else{}	//为了抵消if，防止影响下面的else
			}

			//如果还有物理块可分配
			else if (arr_length > 0) {
				int frame = get_free_frame();	//找一个空闲的物理块
				loca(76, loca_control + 4);
				printf("物理块号：%d", frame);
				if (frame >= 0) {
					arr[frame_count - arr_length] = instruction_page;
					arr_length--;
					(pagetable + instruction_page)->frame = frame;	//页表项中的物理块号改一下
					(pagetable + instruction_page)->state = 1;		//确认该页已被调入内存
					(pagetable + instruction_page)->visitFrequency++;//访问字段自加，暂时没有用
					frame_used[frame] = 1;		//标记该物理块号已被使用
				}
				else{
					loca(76, loca_control + 4);
					printf("无空闲物理块可用");
				}
			}//else if
		}//else

		//计算物理地址，run代表其已被执行
		int physical_address = pageLength * (pagetable + instruction_page)->frame + instruction_shifting;
		loca(76, loca_control + 5);
		run(physical_address);

		//模拟页面修改。随机到1就代表内存中的页面发生了更改，需要写回。
		int flag = (int)rand() % 2;
		if (flag == 1) (pagetable + instruction_page)->change = 1;
		
		//打印页表信息
		for (int k = loca_control; k < loca_control + frame_count - arr_length; k++) {
			loca(2, k);
			//print_pageTableEntry(pagetable, instruction_page);
			print_pageTableEntry(pagetable, arr[frame_count - arr_length - (k - loca_control) - 1]);
		}
		int cc = frame_count - arr_length < 6 ? 6 : frame_count - arr_length;
		loca(0, cc+loca_control); 
		printf("---------------------------------------------------------------------------------------------------------------------");
		
		for (int z = loca_control; z < cc + loca_control; z++) {
			loca(70, z); printf("|");
		}
		loca_control += cc+1;

		//手动控制，一步一步显示变化
		getchar();

	}//for

	loca(40, loca_control + 5); 
	printf("   进程结束");
	loca(40, loca_control + 7); 
	printf("任意键返回菜单");
	char g = getchar();
	free(arr);
	remainingFrame+= frame_count - arr_length;
	menu();
}

//打印页表项(页号、物理块号、状态位、访问字段、修改位）
void print_pageTableEntry(struct pageTableEntry *pagetable,int instruction_page){
	printf("%d\t\t%d\t\t%d\t\t%d\t\t%d", instruction_page, (pagetable + instruction_page)->frame,
		(pagetable + instruction_page)->state, (pagetable + instruction_page)->visitFrequency, (pagetable + instruction_page)->change);
}

//找空闲物理块
int get_free_frame() {
	for (int i = 0; i < frameNum; i++) {
		if (frame_used[i] == 0) return i;
	}
	return -1;
}

//从磁盘写入内存，因为是模拟，所以只打印语句表示执行
void writeToRAM(int instruction_page) {
	printf("将第%d页写入内存", instruction_page);
};

//写入磁盘，因为是模拟，所以只打印语句表示执行
void writeToDisk(int last_recently_used) {
	printf("将第%d页的内容写回磁盘\n",last_recently_used);
};

//执行物理地址的命令
void run(int physical_address) {
	printf("物理地址：%d", physical_address);
}

//处理未初始化内存的情况
void uninitialized_RAM() {
	if (initRAM_flag == 0) {
		system("cls");
		loca(42, 6);  
		printf("__________________________");
		loca(42, 8);  
		printf("|     尚未初始化内存!    |");
		loca(42, 10); 
		printf("|   是否需要初始化内存?  |");
		loca(42, 12); 
		printf("|     1.是       2.否    |");
		loca(42, 13); 
		printf("__________________________");
		loca(42, 15); 
		printf("选择操作：");
		char s;
		while (1) {
			getchar();
			s = getchar();
			switch (s)
			{
			case'1':initRAM(); break;
			case'2':menu(); break;
			default:break;
			}
		}
	}
}

//内存物理块的初始化
void initRAM()
{
	system("cls");
	initRAM_flag = 1;	//标志内存已初始化
	remainingFrame = RAM_Length / pageLength;	//每次初始化内存都是全新的开始
	//刻意制造一些内存块已被使用的情况
	srand(time(NULL));
	for (int i = 0; i < frameNum; i++) {
		if ((frame_used[i] = (int)rand() % 2) == 1)
			remainingFrame--;	
	}
	loca(42, 6); 
	printf("*  内存物理块已经成功初始化  *");
	loca(42, 8); 
	printf("*     共有%d个可用物理块    *",remainingFrame);
	loca(42, 10);
	printf("*       每块大小为%dB      *", pageLength);
	loca(48, 13);
	printf("------------------");
	loca(48, 14);
	printf("|   1.执行程序   |");
	loca(48, 16);
	printf("|   2.返回菜单   |");
	loca(48, 17);
	printf("------------------");
	loca(48, 19);
	printf("请选择操作：");
	char t;
	while (1) {
		t = getchar();
		switch (t)
		{
		case'1':createProcess();break;
		case'2':menu(); break;
		default:break;
			
		}
	}
}

//查看内存
void checkRAM() {
	uninitialized_RAM();	//未初始化内存处理
	system("cls");
	loca(42, 8); printf("*     共有%d个可用物理块    *", remainingFrame);
	loca(42, 10); printf("*       每块大小为%dB      *", pageLength);
	loca(48, 13); printf("------------------");
	loca(48, 14); printf("|   1.执行程序   |");
	loca(48, 16); printf("|   2.返回菜单   |");
	loca(48, 17); printf("------------------");
	loca(48, 19); printf("请选择操作：");
	char t;
	while (1) {
		t = getchar();
		switch (t)
		{
		case'1':createProcess(); break;
		case'2':menu(); break;
		default:break;

		}
	}
}

//退出系统
void over()      //退出系统  
{
	char t;
	system("cls");
	loca(48, 11);
	printf("-----------------------");
	loca(48, 12);
	printf("|   您确定要退出吗?   |");
	loca(48, 14);
	printf("| 1.确定     2.取消   |");
	loca(48, 15);
	printf("-----------------------");
	loca(48, 16);
	while (1)
	{
		t = getchar();
		switch (t)
		{
		case '1':
			system("cls");
			loca(48, 10);
			printf("正在退出....");
			Sleep(1500);
			system("cls");
			loca(48, 10);
			printf("已退出软件");
			loca(48, 12);
			printf("谢谢使用！");
			Sleep(1000);
			loca(48, 14);
			exit(0);  break;      //终止程序   
		case '2':
			menu(); break;   //调用函数，进入菜单   
		default:break;
		}
	}
}

//将光标移动到x,y坐标处  
void loca(int x, int y)
{
	COORD pos = { x,y };          //COORD是Windows API中定义的一种结构，表示一个字符在控制台屏幕上的坐标
	HANDLE Out = GetStdHandle(STD_OUTPUT_HANDLE); //GetStdHandle（）返回标准的输入、输出或错误的设备的句柄，也就是获得输入、输出/错误的屏幕缓冲区的句柄
	SetConsoleCursorPosition(Out, pos); //API中定位光标位置的函数
}

int main()
{
	menu();
	return 0;
}

