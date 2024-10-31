#include <iostream>
#include <cassert>
#include <assert.h>

using namespace std;

static const int SIZE = 5;
int a[SIZE];        // 原始数组
int d[SIZE << 2];   // 线段树数组, 0下标不用

int b[SIZE << 2];   //懒标记数组, 0下标不用

//构建线段树
void build(int s, int t, int p) {
  // 对 [s,t] 区间建立线段树,当前根的编号为 p
  if (s == t) {
    d[p] = a[s];
    return;
  }
  int m = s + ((t - s) >> 1);
  // 移位运算符的优先级小于加减法，所以加上括号
  // 如果写成 (s + t) >> 1 可能会超出 int 范围
  build(s, m, p * 2);           // 左子树
  build(m + 1, t, p * 2 + 1);   // 右子树
  // 递归对左右区间建树
  d[p] = d[p * 2] + d[(p * 2) + 1];
}

//用于不需要lazy标记的查询
int getsum1(int l, int r, int s, int t, int p) {
  // [l, r] 为查询区间, [s, t] 为当前节点包含的区间, p 为当前节点的编号
  if (l <= s && t <= r)
    return d[p];  // 当前区间为询问区间的子集时直接返回当前区间的和
  int m = s + ((t - s) >> 1), sum = 0;
  if (l <= m) 
    sum += getsum1(l, r, s, m, p * 2);
  // 如果左儿子代表的区间 [s, m] 与询问区间有交集, 则递归查询左儿子
  if (r > m) 
    sum += getsum1(l, r, m + 1, t, p * 2 + 1);
  // 如果右儿子代表的区间 [m + 1, t] 与询问区间有交集, 则递归查询右儿子
  return sum;
}


// [l, r] 为修改区间, c 为被修改的元素的变化量, [s, t] 为当前节点包含的区间, p
// 为当前节点的编号
//更新区间，lazy标记，更新线段树
void update(int l, int r, int c, int s, int t, int p) {
  // 当前区间为修改区间的子集时直接修改当前节点的值,然后打标记,结束修改
  if (l <= s && t <= r) {
    d[p] += (t - s + 1) * c;
    b[p] += c;
    return;
  }
  //中点
  int m = s + ((t - s) >> 1);
  if (b[p] && s != t) {
    // 如果当前节点的懒标记非空,则更新当前节点两个子节点的值和懒标记值
    d[p * 2] += b[p] * (m - s + 1);
    d[p * 2 + 1] += b[p] * (t - m);

    b[p * 2] += b[p];      // 将标记下传给子节点
    b[p * 2 + 1] += b[p];  // 将标记下传给子节点
    b[p] = 0;     // 清空当前节点的标记
  }

  if (l <= m) update(l, r, c, s, m, p * 2);

  if (r > m) update(l, r, c, m + 1, t, p * 2 + 1);

  d[p] = d[p * 2] + d[p * 2 + 1];

}

//用于lazy标记的查询
//lazy标记在查询时更新线段树的值和懒标记,然后清空懒标记
int getsum2(int l, int r, int s, int t, int p) {
  // [l, r] 为查询区间, [s, t] 为当前节点包含的区间, p 为当前节点的编号
  if (l <= s && t <= r) {
    return d[p];
  }
  // 当前区间为询问区间的子集时直接返回当前区间的和
  int m = s + ((t - s) >> 1);

  if (b[p]) {
    // 如果当前节点的懒标记非空,则更新当前节点两个子节点的值和懒标记值
    d[p * 2] += b[p] * (m - s + 1);
    d[p * 2 + 1] += b[p] * (t - m);
    
    b[p * 2] += b[p];
    b[p * 2 + 1] += b[p];  // 将标记下传给子节点
    b[p] = 0;              // 清空当前节点的标记

  }

  int sum = 0;

  if (l <= m) 
    sum = getsum2(l, r, s, m, p * 2);
  if (r > m) 
    sum += getsum2(l, r, m + 1, t, p * 2 + 1);

  return sum;
}

void Test() {
    // 初始化数组
    for(int i = 1; i < SIZE; i++) {
        a[i] = i;
    }
    // 1 = 左边界，SIZE - 1 = 右边界，1 = 根节点编号
    build(1, SIZE - 1, 1);
    //cout << "getsum1(1, 99, 1, SIZE - 1, 1) = " << getsum1(1, 99, 1, SIZE - 1, 1) << endl;
    //cout << "getsum1(2, 29, 1, SIZE - 1, 1) = " << getsum1(2, 29, 1, SIZE - 1, 1) << endl;
    //assert(getsum1(1, 99, 1, SIZE - 1, 1) == 4950);
    //assert(getsum1(2, 29, 1, SIZE - 1, 1) == 434);
    cout << "getsum1(2, 4, 1, SIZE - 1, 1) = " << getsum1(2, 4, 1, SIZE - 1, 1) << endl;
    cout << "update(1, 4, 1, 1, SIZE - 1, 1)" << endl;
    update(1, 4, 1, 1, SIZE - 1, 1);
    cout << "getsum1(2, 4, 1, SIZE - 1, 1) = " << getsum2(2, 4, 1, SIZE - 1, 1) << endl;
}

int main() {
    Test();
    return 0;
}