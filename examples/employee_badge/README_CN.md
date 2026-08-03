# 电子工牌示例

本示例为 T5 E-Paper Basic 绘制一张 540 × 960 的竖屏电子工牌。页面参考
`other/work.png`，人物取自 `other/logo.png`，其余内容由 M5GFX 图元和字体绘制。
图片会在编译时嵌入固件，运行时不依赖 SD 卡或外部文件系统。

页面包括：

- LILYGO 品牌标题和线路装饰
- 由 `other/logo.png` 裁切、缩放得到的人物图像
- 姓名、部门和工号信息
- 可扫描的网址二维码
- 网站及品牌页脚

## 修改工牌资料

编辑 `main.cpp` 开头的以下常量：

```cpp
constexpr char kEmployeeName[] = "Your Name Here";
constexpr char kDepartment[] = "Your Department Here";
constexpr char kEmployeeId[] = "LGO-00000";
constexpr char kWebsite[] = "www.lilygo.cc";
constexpr char kQrPayload[] = "https://www.lilygo.cc/";
```

`kQrPayload` 是二维码实际编码的内容，可以改为员工主页、联系方式或设备管理地址。

## 编译

确认 `platformio.ini` 中：

```ini
src_dir = examples/employee_badge
```

然后执行：

```powershell
pio run -e T5_E_PAPER_BASIC
```

程序启动后使用高质量全屏刷新显示一次工牌，之后不再周期刷新。

`logo_asset.S` 会在编译时把 `other/logo.png` 直接嵌入固件，因此运行时不需要
SD 卡或文件系统。替换 `logo.png` 后建议先删除旧构建缓存或执行一次
`pio run -t clean`，再重新编译。
