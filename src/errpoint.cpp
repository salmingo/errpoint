/*
 Name        : errpoint.cpp
 Author      : Xiaomeng Lu
 Date        : 12-24-2019
 Description : 从日志文件中统计GWAC指向偏差
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string.h>
#include <boost/filesystem.hpp>
#include "ADefine.h"
#include "ATimeSpace.h"

using std::vector;
using std::string;
using namespace AstroUtil;
using namespace boost::filesystem;

ATimeSpace ats;
double lon = 117.57454;  // 地理经度, 东经为正
double lat =  40.39593;  // 地理纬度, 北纬为正
double alt = 910;        // 海拔高度
double tz  = 8;  // 时区
double fd0 = tz / 24.0;

/*!
 * @struct pt_line 行数据
 */
struct pt_line {
	string devid;
	int hh, mm;  // 本地时
	double ss;
	double obj_ra, obj_dc;   // 目标位置, 量纲: 角度
	double sky_ra, sky_dc;   // 天文位置, 量纲: 角度
};

/*!
 * @brief 解析一个log文件
 * @note
 * 文件名格式: gtoaes_ccyymmdd.log
 */
void resolve_logfile(string &pathdir, string &filename) {
	int ccyy, mm, dd;   // 本地时
	string devid_old;
	FILE *fpdst;
	// 从文件名取日期

	// 遍历行数据
	path filepath = pathdir;
	double mjd, lmst, ha_obj;
	filepath /= filename;
	FILE *fplog = fopen(filepath.c_str(), "r");
	if (!fplog) {
		printf ("failed to access file : %s\n", filename.c_str());
		return;
	}

	while (!feof(fplog)) {

		// 计算恒星时

		// 输出结果
	}

	fclose(fplog);
}

/*!
 *
 */
bool resolve_line(const char *line, pt_line &pt) {
	const char valid_line[] = "legal guide<"; // 有效行
	const char tag_obj[] = "object: <";
	const char tag_sky[] = "sky: <";
	char *ptr;

	ptr = strstr(line, valid_line);
	if (!ptr) return false;

	// 从行首取时间

	// 从ptr取gid:uid

	ptr = strstr(line, tag_obj);
	// 从ptr取目标位置

	ptr = strstr(line, tag_sky);
	// 从ptr取天球位置

	return true;
}

int main(int argc, char **argv) {
	// 遍历目录下所有.log文件

	// 从行数据中提取信息, 信息体以<>作为分隔符
	// 行起始为时标, 格式: hh:mm:ss, 本地时
	// legal guide: 设备编号, gid:uid
	// object: 目标位置, ra, dec
	// sky: 天球位置, ra, dec

	// 按照设备编号生成结果文件, 结果文件名为err_gid-uid.txt
	// 文件内容:
	// UTC时标
	// 恒星时
	// 目标位置: 赤经，赤纬，时角，量纲：角度
	// 天球位置: 赤经，赤纬
	// 指向偏差：目标位置-天球位置，量纲：角秒

	return 0;
}
