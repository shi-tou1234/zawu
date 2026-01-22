#include <stdio.h>
#include <stdlib.h>//后续清屏调用system函数，随机数rand等 
#include <string.h>
#include <time.h>//初始化随机数种子 
/* 棋盘大小 */
#define BOARD_SIZE 15//后续可以通过修改这个数值来改变棋盘大小 
/* 最大历史记录数 */
#define MAX_HISTORY (BOARD_SIZE * BOARD_SIZE)//用来判别是否和棋 
/* 棋盘元素索引（文件参考代码） */
#define TAB_TOP_LEFT 0x0       // 左上角 ┏
#define TAB_TOP_CENTER 0x1     // 顶部边缘 ┯
#define TAB_TOP_RIGHT 0x2      // 右上角 ┓
#define TAB_MIDDLE_LEFT 0x3    // 左侧边缘 ┠
#define TAB_MIDDLE_CENTER 0x4  // 中间交叉点 ┼
#define TAB_MIDDLE_RIGHT 0x5   // 右侧边缘 ┨
#define TAB_BOTTOM_LEFT 0x6    // 左下角 ┗
#define TAB_BOTTOM_CENTER 0x7  // 底部边缘 ┷
#define TAB_BOTTOM_RIGHT 0x8   // 右下角 ┛
#define CHESSMAN_BLACK 0x9     // 黑棋 ●
#define CHESSMAN_WHITE 0xA     // 白棋 ○
/* 清屏命令 （文件参考代码）*/
#undef CLS
#ifdef WIN32
#  define CLS "cls"
#else
#  define CLS "clear"
#endif
/* 布尔值定义 - 兼容性(这段代码是一个很神奇的存在，
如果不加，那么在vscode上面不能运行，但是在dev和clion可以运行，
询问ai之后，添加了这行，可能是编译器不同的原因,为了可移植性，就加上了）*/
#ifndef __cplusplus
#  ifndef true
#    define true 1
#  endif
#  ifndef false
#    define false 0
#  endif
#endif
/* 坐标结构体（确定棋子的位置）文件参考代码 */
typedef struct {
    int x, y;
} CHESS_POINT;
/* 历史记录结构体（用来悔棋） */
typedef struct {
    CHESS_POINT pos;     /* 落子位置 */
    int color;          /* 记录是谁下的 */
    int original;       /* 记录落子之前这个格子是什么，用来恢复原装 */
} HISTORY;
/* 函数指针类型 （依据这个指针来确定棋子的位置，让电脑确定价值位置）*/
typedef CHESS_POINT (*GET_POINT_FUNC)(int color);
/* 棋盘显示元素 - UTF-8编码（文件参考代码） */
const char* element[] = {
    "┏",    // 0 - top left
    "┯",    // 1 - top center
    "┓",    // 2 - top right
    "┠",    // 3 - middle left
    "┼",    // 4 - middle center
    "┨",    // 5 - middle right
    "┗",    // 6 - bottom left
    "┷",    // 7 - bottom center
    "┛",    // 8 - bottom right
    "●",    // 9 - black
    "○"     // 10 - white
};
/* 横线元素 */
const char* hline = "─";
/* 棋盘数组 （边界）文件参考代码*/
// 大小是 BOARD_SIZE + 2，因为需要额外的一圈存边框（0行/列 和 18行/列）
// 有效下棋区域是 1 到 17
int chessboard[BOARD_SIZE + 2][BOARD_SIZE + 2];
/* 历史记录 */
HISTORY history[MAX_HISTORY];   // 存储每一步记录的数组
int history_count = 0;          // 当前总步数计数器
/* 悔棋请求标志 */
int undo_requested = 0;         // 标记用户是否发起了悔棋请求 (1=是, 0=否)
int undo_steps = 0;             // 用户想悔多少步
/* 方向数组：横、撇、竖、捺（文件参考代码）检查是否已经结束棋局 */
const int dir[4][2] = {
    {0, -1},    // 横
    {-1, -1},   // 撇
    {-1, 0},    // 竖
    {-1, 1}     // 捺
};
/* 宏定义：判断坐标是否在棋盘内 */
#define IN_BOARD(x, y) ((x) >= 1 && (x) <= BOARD_SIZE && (y) >= 1 && (y) <= BOARD_SIZE)
/* 宏定义：判断两个位置棋子颜色是否相同 */
#define EQUAL(x1, y1, x2, y2) (chessboard[x1][y1] == chessboard[x2][y2])
/* 函数声明 */
// 初始化棋盘
void show_chessboard(void);        // 显示棋盘
int choice(void);                  // 菜单选择
CHESS_POINT from_user(int color);  // 用户输入坐标
CHESS_POINT from_computer(int color); // 电脑计算坐标
int has_end(CHESS_POINT p);        // 判断是否连成5子
int calc_value(CHESS_POINT p, int color); // AI估值函数
void play_game(int mode);          // 游戏主逻辑
void add_history(CHESS_POINT p, int color, int original); // 添加记录
int undo_move(int steps);          // 执行悔棋
void clear_input_buffer(void);     // 清理输入流
/* 初始化棋盘 */
void init_chessboard(void)
{
    int w, h;
    /*四个角*/
    chessboard[0][0] = TAB_TOP_LEFT;
    chessboard[0][BOARD_SIZE + 1] = TAB_TOP_RIGHT;
    chessboard[BOARD_SIZE + 1][0] = TAB_BOTTOM_LEFT;
    chessboard[BOARD_SIZE + 1][BOARD_SIZE + 1] = TAB_BOTTOM_RIGHT;
    /*顶部边缘，打印棋盘边缘轮廓 */
    for (w = 1; w <= BOARD_SIZE; w++) {
        chessboard[0][w] = TAB_TOP_CENTER;
    }
    /*底部边缘，同上*/
    for (w = 1; w <= BOARD_SIZE; w++) {
        chessboard[BOARD_SIZE + 1][w] = TAB_BOTTOM_CENTER;
    }
    /* 左侧边缘，同上*/
    for (h = 1; h <= BOARD_SIZE; h++) {
        chessboard[h][0] = TAB_MIDDLE_LEFT;
    }
    /*右侧边缘，同上*/
    for (h = 1; h <= BOARD_SIZE; h++) {
        chessboard[h][BOARD_SIZE + 1] = TAB_MIDDLE_RIGHT;
    }
    /* 中间区域，打印中间中间部分的棋盘*/
    for (h = 1; h <= BOARD_SIZE; h++) {
        for (w = 1; w <= BOARD_SIZE; w++) {
            chessboard[h][w] = TAB_MIDDLE_CENTER;// 初始化为 '┼'
        }
    }
}
/* 显示棋盘（文件参考代码）*/
void show_chessboard(void)
{
    int w, h;
    system(CLS);
#ifdef WIN32
    system("color F0");
#endif
    /* 打印列号 */
    printf("   ");  
    for (w = 1; w <= BOARD_SIZE; w++) {
        printf("%2d", w);
    }
    printf("\n");
    /* 打印棋盘 */
    for (h = 0; h <= BOARD_SIZE + 1; h++) {
    	// 如果是中间行，打印行号(这里调试了很久，要让数字和线一一对齐）
        if (h >= 1 && h <= BOARD_SIZE) {
            printf("%2d", h);
        } else {
            printf("  ");// 边框行不打印行号，补空格
        }
        // 打印每一行的所有列
        for (w = 0; w <= BOARD_SIZE + 1; w++) {
            printf("%s", element[chessboard[h][w]]);
            // 如果不是最后一列，打印横线连接
            if (w < BOARD_SIZE + 1) {
                printf("%s", hline);
            }
        }
        printf("\n");// 每一行打印完换行
    }
    printf("\n");
}
/* 选择游戏模式 */
int choice(void)
{
    int res = 0;
    // 打印菜单界面
    printf("\n");
    printf("╔════════════════════════════╗\n");
    printf("║       五 子 棋 游 戏       ║\n");
    printf("╠════════════════════════════╣\n");
    printf("║  1) 计算机(黑) vs 人(白)   ║\n");
    printf("║  2) 人(黑) vs 计算机(白)   ║\n");
    printf("║  3) 人(黑) vs 人(白)       ║\n");
    printf("║  0) 退出游戏               ║\n");
    printf("╚════════════════════════════╝\n");
    printf("\n请选择游戏模式: ");
    if (scanf("%d", &res) != 1) {
        /* 清空输入缓冲区 */
        while (getchar() != '\n');// 清空输入缓冲区，防止死循环
        return -1;
    }
// 清空输入缓冲区中残留的换行符，防止影响后续的 fgets 
    while (getchar() != '\n');
    return res;// 返回用户选择的模式
}
/* 判断游戏是否结束 （文件参考代码）*/
int has_end(CHESS_POINT p)
{
    int d;       // 方向索引
    int count;   // 连子计数
    int x, y;    // 临时坐标
    // 遍历 4 个方向：横、竖、撇、捺
    for (d = 0; d < 4; d++) {
        count = 1;  /* 包含当前落子 */
        /* 正方向检查 */
        x = p.x + dir[d][0];
        y = p.y + dir[d][1];
        // 只要不出界且颜色相同，就继续数
        while (IN_BOARD(x, y) && EQUAL(p.x, p.y, x, y)) {
            count++;
            x += dir[d][0];
            y += dir[d][1];
        }
        /* 反方向检查 */
        x = p.x - dir[d][0];
        y = p.y - dir[d][1];
        //同上 
        while (IN_BOARD(x, y) && EQUAL(p.x, p.y, x, y)) {
            count++;
            x -= dir[d][0];
            y -= dir[d][1];
        }
        // 如果某个方向连成了5个或以上，游戏结束，获胜 
        if (count >= 5) {
            return true;
        }
    }
    // 还没赢
    return false;
}
/* 计算某位置的价值（文件参考代码）*/
int calc_value(CHESS_POINT p, int color)
{
    /* 基础分值表：连1个、2个...5个的分数 */
    static const int values[] = {
        0, 10, 100, 1000, 10000
    };
    /* 棋盘中心坐标 (9) */
    static const int center = BOARD_SIZE / 2 + 1;
    
    int d, count_self, count_opp;
    int x, y;
    int sum = 0;
    int blocked_self, blocked_opp;
     //opponent 是对手颜色
    int opponent = (color == CHESSMAN_BLACK) ? CHESSMAN_WHITE : CHESSMAN_BLACK;
    /* 计算四个方向的价值 */
    for (d = 0; d < 4; d++) {
          /* 进攻分数：看如果下在这里，电脑能连多少 --- */
        count_self = 0;
        blocked_self = 0;// blocked 表示两端是否被堵住
        /* 正方向 */
        x = p.x + dir[d][0];
        y = p.y + dir[d][1];
        while (IN_BOARD(x, y) && chessboard[x][y] == color) {
            count_self++;
            x += dir[d][0];
            y += dir[d][1];
        }
        // 检查端点状态：如果出界了，或者碰到了不是空地也不是自己棋子的东西（即被堵了）
        if (!IN_BOARD(x, y) || (chessboard[x][y] != TAB_MIDDLE_CENTER && 
                                chessboard[x][y] != TAB_MIDDLE_LEFT &&
                                chessboard[x][y] != TAB_MIDDLE_RIGHT &&
                                chessboard[x][y] != TAB_TOP_CENTER &&
                                chessboard[x][y] != TAB_BOTTOM_CENTER &&
                                chessboard[x][y] != color)) {
            blocked_self++;
        }
        
        /* 反方向 */
        x = p.x - dir[d][0];
        y = p.y - dir[d][1];
        while (IN_BOARD(x, y) && chessboard[x][y] == color) {
            count_self++;
            x -= dir[d][0];
            y -= dir[d][1];
        }
        if (!IN_BOARD(x, y) || (chessboard[x][y] != TAB_MIDDLE_CENTER && 
                                chessboard[x][y] != TAB_MIDDLE_LEFT &&
                                chessboard[x][y] != TAB_MIDDLE_RIGHT &&
                                chessboard[x][y] != TAB_TOP_CENTER &&
                                chessboard[x][y] != TAB_BOTTOM_CENTER &&
                                chessboard[x][y] != color)) {
            blocked_self++;
        }
        
 /*防守分数：看如果下在这里，能破坏对手多少连子*/
        count_opp = 0;
        blocked_opp = 0;
         //逻辑同上，只是统计的是opponent的子
        /* 正方向 */
        x = p.x + dir[d][0];
        y = p.y + dir[d][1];
        while (IN_BOARD(x, y) && chessboard[x][y] == opponent) {
            count_opp++;
            x += dir[d][0];
            y += dir[d][1];
        }
        if (!IN_BOARD(x, y) || (chessboard[x][y] != TAB_MIDDLE_CENTER && 
                                chessboard[x][y] != TAB_MIDDLE_LEFT &&
                                chessboard[x][y] != TAB_MIDDLE_RIGHT &&
                                chessboard[x][y] != TAB_TOP_CENTER &&
                                chessboard[x][y] != TAB_BOTTOM_CENTER &&
                                chessboard[x][y] != opponent)) {
            blocked_opp++;
        }
        
        /* 反方向 */
        x = p.x - dir[d][0];
        y = p.y - dir[d][1];
        while (IN_BOARD(x, y) && chessboard[x][y] == opponent) {
            count_opp++;
            x -= dir[d][0];
            y -= dir[d][1];
        }
        if (!IN_BOARD(x, y) || (chessboard[x][y] != TAB_MIDDLE_CENTER && 
                                chessboard[x][y] != TAB_MIDDLE_LEFT &&
                                chessboard[x][y] != TAB_MIDDLE_RIGHT &&
                                chessboard[x][y] != TAB_TOP_CENTER &&
                                chessboard[x][y] != TAB_BOTTOM_CENTER &&
                                chessboard[x][y] != opponent)) {
            blocked_opp++;
        }
        
        /* 计算己方价值 */
        if (count_self > 0 && count_self < 5) {
            if (blocked_self == 0) {
                sum += values[count_self] * 2;  // 两头都没堵，是“活棋”，分数加倍
            } else if (blocked_self == 1) {    
                sum += values[count_self];      // 堵了一头，是“冲棋”，给基础分
            }
            /* blocked_self == 2 时为死棋，没用，不加分 */
        } else if (count_self >= 5) {
            sum += 100000;  /* 五连，直接给超高分（必胜）*/
        }
        /* 计算防守价值（阻挡对方） */
        if (count_opp > 0 && count_opp < 5) {
            if (blocked_opp == 0) {
                sum += values[count_opp] * 1.5;  // 对手有活棋，威胁大，必须防守，给较高分
            } else if (blocked_opp == 1) {
                sum += values[count_opp] * 0.8; // 对手被堵一头，威胁稍小
            }
        } else if (count_opp >= 4) {
            sum += 80000;  // 对手已经4连了，不堵就输了，给仅次于必胜的高分
        }
    }
/* 位置加成：越靠近中心越好 */
// 算法：中心坐标 - 距离中心的距离
    sum += (center - abs(center - p.x)) * (center - abs(center - p.y));
    
    return sum;
}
/* 判断位置是否为空 */
int is_empty(int x, int y)
{
    int val = chessboard[x][y];
    // 只要不是黑棋也不是白棋，就是空的（可能是十字线符号） 
    return (val != CHESSMAN_BLACK && val != CHESSMAN_WHITE);
}
/* 清空输入缓冲区 */
void clear_input_buffer(void)
{
    int c;
    // 循环读取直到遇到换行符或文件结束符
    while ((c = getchar()) != '\n' && c != EOF);
}

/* 添加历史记录 （实现悔棋功能）*/
void add_history(CHESS_POINT p, int color, int original)
{
    if (history_count < MAX_HISTORY) { // 防止溢出
        history[history_count].pos = p;
        history[history_count].color = color;
        history[history_count].original = original;// 必须保存原始值，否则悔棋后棋盘符号会乱
        history_count++;
    }
}
/* 悔棋：撤销指定步数 */
int undo_move(int steps)
{
    int i;
    HISTORY* h;
    
    if (steps <= 0 || steps > history_count) {
        return 0; // 失败，没悔成
    }
    
    for (i = 0; i < steps; i++) {
        history_count--;// 指针回退
        h = &history[history_count];
        // 核心：将棋盘该位置恢复成原来的符号（比如 '┼' 或 '┗'）
        chessboard[h->pos.x][h->pos.y] = h->original; 
    }
    return steps;  /* 返回实际悔棋步数 */
}
/* 玩家落子 */
CHESS_POINT from_user(int color)
{
    CHESS_POINT p = {0, 0};
    int valid = false;// 标记输入是否有效
    const char* color_name = (color == CHESSMAN_BLACK) ? "黑棋" : "白棋";
    char input[64];
     // 初始化悔棋标记，默认不悔棋
    undo_requested = 0;
    undo_steps = 0;
    while (!valid) {
        printf("%s落子，请输入坐标 (行 列), 'u [步数]' 悔棋, 'q' 退出: ", color_name);
        // 使用 fgets 读取整行，防止输入空格导致 scanf 异常（fgets是自学使用的，参考了csdn的教程 
        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;
        }
        // 将末尾的换行符替换为结束符 \0
        input[strcspn(input, "\n")] = 0;
        /* 检查是否是退出命令 */
        if (input[0] == 'q' || input[0] == 'Q') {
            p.x = -3;  // 使用特殊坐标 (-3, -3) 代表退出
            p.y = -3;
            return p;
        }
        /* 检查是否是悔棋命令 */
        if (input[0] == 'u' || input[0] == 'U') {
            int steps = 1;// 尝试读取 u 后面的数字，如 "u 2"
            if (sscanf(input + 1, "%d", &steps) != 1) {
                steps = 1;// 没读到数字默认悔1步
            }
            if (steps < 1) steps = 1;
             // 设置全局标志位
            undo_requested = 1;
            undo_steps = steps;
            p.x = -2;// 使用特殊坐标 (-2, -2) 代表悔棋
            p.y = -2;
            return p;
        }
        
        /* 解析坐标 */
        if (sscanf(input, "%d %d", &p.x, &p.y) != 2) {
            printf("输入格式错误！请输入两个数字、'u' 悔棋 或 'q' 退出\n");
            continue;
        }
        // 检查越界
        if (!IN_BOARD(p.x, p.y)) {
            printf("坐标超出范围！请输入1-%d之间的数字\n", BOARD_SIZE);
            continue;
        }
        // 检查该位置是否已有子
        if (!is_empty(p.x, p.y)) {
            printf("该位置已有棋子！请重新输入\n");
            continue;
        }
        
        valid = true;
    }
    
    return p;
}

/* 计算机落子 */
CHESS_POINT from_computer(int color)
{
    CHESS_POINT p = {0, 0}, best = {0, 0};
    int max_value = -1, value;
    int first_move = true;
    
     //检查是否是第一步
    // 遍历全棋盘，如果有任何子，first_move 就设为 false
    for (p.x = 1; p.x <= BOARD_SIZE; p.x++) {
        for (p.y = 1; p.y <= BOARD_SIZE; p.y++) {
            if (!is_empty(p.x, p.y)) {
                first_move = false;
                break;
            }
        }
        if (!first_move) break;
    }
    
    /* 第一步下中心 */
    if (first_move) {
        best.x = BOARD_SIZE / 2 + 1;//9
        best.y = BOARD_SIZE / 2 + 1;//9
        printf("计算机落子: %d %d\n", best.x, best.y);
        return best;
    }
    
    /* 遍历所有空位，计算价值 */
    for (p.x = 1; p.x <= BOARD_SIZE; p.x++) {
        for (p.y = 1; p.y <= BOARD_SIZE; p.y++) {
            if (is_empty(p.x, p.y)) {// 如果是空的
            // 调用估值函数
                value = calc_value(p, color);
                
                /* 增加一些随机性，避免下棋过于呆板 */
                value += rand() % 5;
                
                if (value > max_value) {
                    max_value = value;
                    best = p;
                }
            }
        }
    }
    
    printf("计算机落子: %d %d\n", best.x, best.y);
    
    return best;
}

/* 游戏主循环 */
void play_game(int mode)
{
	// get_point 是一个函数指针数组，存储两个函数：
	//黑棋落子函数 和 白棋落子函数
    GET_POINT_FUNC get_point[2];
    int who = 0;  /* 0: 黑棋先手, 1: 白棋 */
    int color;
    CHESS_POINT p;
    int game_over = false;
    int total_moves = 0;
    const int max_moves = BOARD_SIZE * BOARD_SIZE;
    int original_value;
    
    /* 根据模式设置落子函数 */
    switch (mode) {
        case 1:  /* 计算机(黑) vs 人(白) */
            get_point[0] = from_computer; // 黑棋由电脑下
            get_point[1] = from_user;     // 白棋由人下（下面同下） 
            break;
        case 2:  /* 人(黑) vs 计算机(白) */
            get_point[0] = from_user;
            get_point[1] = from_computer;
            break;
        case 3:  /* 人(黑) vs 人(白) */
            get_point[0] = from_user;
            get_point[1] = from_user;
            break;
        default:
            return;
    }
    
    /* 初始化棋盘和历史记录 */
    init_chessboard();
    history_count = 0;
    
    /* 游戏主循环 */
    while (!game_over && total_moves < max_moves) {
        /* 确定当前棋子颜色 */
        color = (who == 0) ? CHESSMAN_BLACK : CHESSMAN_WHITE;
        
        /* 显示棋盘 */
        show_chessboard();
        // 调用对应的函数获取落子位置
        // 如果 who=1且模式1，这里实际上调用的是 from_user(color)
        /* 获取落子位置 */
        p = (*get_point[who])(color);
        
        /* 检查是否请求退出 */
        if (p.x == -3 && p.y == -3) {
            printf("\n游戏已退出！\n");
            printf("\n按回车键返回主菜单...");
            getchar();
            return;
        }
        
        /* 检查是否请求悔棋 */
        if (p.x == -2 && p.y == -2) {
            if (history_count == 0) {  //校验玩家是否下过棋
                printf("\n没有可以悔的棋！\n");
                printf("按回车继续...");
                getchar();
                continue;
            }
            
            /* 计算实际悔棋步数 */
            int actual_steps = undo_steps;
            int user_steps = undo_steps;  /* 记录用户请求的步数 */
            
            /* 在人机对战中，悔棋需要撤销两步（自己和对方各一步） */
            if (mode == 1 || mode == 2) {
                /* 检查玩家是否下过棋 */
                int player_moves = 0;
                int i;
                int player_color = (mode == 1) ? CHESSMAN_WHITE : CHESSMAN_BLACK;
                for (i = 0; i < history_count; i++) {
                    if (history[i].color == player_color) {
                        player_moves++;
                    }
                }
                
                if (player_moves == 0) {
                    printf("\n您还没有下过棋，无法悔棋！\n");
                    printf("按回车继续...");
                    getchar();
                    continue;
                }
                
                /* 限制悔棋步数不超过玩家已下的步数 */
                if (user_steps > player_moves) {
                    user_steps = player_moves;
                }
                
                actual_steps = user_steps * 2;  /* 撤销自己和电脑的步 */
                if (actual_steps > history_count) {
                    actual_steps = history_count;
                }
            } else {
                /* 人人对战中，按请求步数悔棋 */
                if (actual_steps > history_count) {
                    actual_steps = history_count;
                }
                user_steps = actual_steps;
            }
            
            int undone = undo_move(actual_steps);
            total_moves -= undone;
            
            /* 在人人对战中，悔棋后需要调整当前玩家 */
            if (mode == 3 && undone > 0) {
                /* 如果悔了奇数步，需要切换玩家 */
                if (undone % 2 == 1) {
                    who = 1 - who;
                }
            }
            /* 人机对战中，悔偶数步，玩家不变 */
            
            printf("\n悔棋成功！\n");
            printf("按回车继续...");
            getchar();
            continue;
        }
        
        /* 正常落子逻辑 */
        original_value = chessboard[p.x][p.y]; // 记住这里原来是什么符号
        chessboard[p.x][p.y] = color;          // 修改为棋子
        add_history(p, color, original_value); // 记入历史
        total_moves++;
        
        /* 判断是否结束 */
        if (has_end(p)) {
            game_over = true;
            show_chessboard();
    
            if (color == CHESSMAN_BLACK) {
                printf("游戏结束！黑棋获胜！\n");
            } else {
                printf("游戏结束！白棋获胜！\n");
            }
        } else if (total_moves >= max_moves) {
            show_chessboard();
            printf("游戏结束！平局！\n");
        }
        
        /* 切换玩家 */
        who = 1 - who;
    }
    
    printf("\n按回车键返回主菜单...");
    getchar();  /* 消耗之前的换行符 */
}

/* 主函数 */
int main(void)
{
    int mode;
    
      /* 初始化随机数种子 - 使用当前时间戳 */
    // 如果不加这一行，每次运行程序电脑下的棋都会一模一样
    srand((unsigned int)time(NULL));
    
    while (1) { // 死循环，直到用户选择退出
        system(CLS);    // 清屏
#ifdef WIN32
        system("color F0");  // Windows下设置白底黑字
#endif
        mode = choice(); // 获取用户选择
        
        if (mode == 0) {
            printf("\n感谢游玩，再见！\n");
            break;// 退出循环，结束程序
        }
        
        if (mode >= 1 && mode <= 3) {
            play_game(mode);// 开始游戏
        } else {
            printf("\n无效选项，请重新选择！\n");
            printf("按回车键继续...");
            getchar(); // 处理无效输入
        }
    }
    
    return 0;
}

