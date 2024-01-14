import pandas as pd
import sys
import pandas as pd

# 获取命令行参数
csv_path = sys.argv[1]

try:
    # 使用pandas读取csv文件, index_col=False, header=None
    df = pd.read_csv(csv_path, index_col=False, header=None)
    
    # 描述DataFrame
    df_description = df.describe()
    
    print(df_description)
except FileNotFoundError:
    print("文件路径错误，请重新输入正确的csv路径。")
