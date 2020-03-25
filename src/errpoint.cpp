/**
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
double lon  = 117.57454;  // 地理经度, 东经为正
double rlon = lon * D2R;  // 以弧度表示的地理经度
double lat  =  40.39593;  // 地理纬度, 北纬为正
double alt = 910;         // 海拔高度
int tz = 8;  // 时区
double fd0 = tz / 24.0;

/*!
 * @struct pt_line 行数据
 */
struct pt_line {
	string devid;
	int hh, mm, ss;  // 本地时
	double obj_ra, obj_dc;   // 目标位置, 量纲: 角度
	double sky_ra, sky_dc;   // 天文位置, 量纲: 角度
};

/*!
 * @struct last_point 最新一次指向
 */
struct last_point {
	string devid;
	double ra, dc;

public:
	/*!
	 * @brief 检查是否新的指向
	 * @return
	 * 0 : 新的指向
	 * 1 : 相同指向
	 * 2 : 设备编号不匹配
	 */
	int is_last(const string &id, double ra_pt, double dc_pt) {
		if (id != devid) return 2;
		if (ra_pt == ra && dc_pt == dc) return 1;
		ra = ra_pt;
		dc = dc_pt;
		return 0;
	}
};

vector<last_point> devls;  // 最后一次指向设备列表

/*!
 *
 */
bool resolve_line(char *line, pt_line &pt) {
	const char *valid_line = "legal guide<"; // 有效行
	const char *tag_obj = "object: <";
	const char *tag_sky = "sky: <";
	char *ptr;
	char tmp[20];
	int i, j;

	ptr = strstr(line, valid_line);
	if (!ptr) return false;
	// 从行数据中提取信息, 信息体以<>作为分隔符
	// 行起始为时标, 格式: hh:mm:ss, 本地时
	// 从行首取时间
	tmp[0] = line[0], tmp[1] = line[1], tmp[2] = 0;
	pt.hh = atoi(tmp);
	tmp[0] = line[3], tmp[1] = line[4];
	pt.mm = atoi(tmp);
	tmp[0] = line[6], tmp[1] = line[7];
	pt.ss = atoi(tmp);

	// legal guide: 设备编号, gid:uid
	// 从ptr取gid:uid
	ptr += strlen(valid_line);
	for (i = 0; ptr[i] != '>'; ++i) tmp[i] = ptr[i];
	tmp[i] = 0;
	for (j = 0; j < i && tmp[j] != ':'; ++j);
	tmp[j] = '-';
	pt.devid = tmp;

	// object: 目标位置, ra, dec
	// 从ptr取目标位置
	ptr = strstr(line, tag_obj);
	if (!ptr) return false;
	ptr += strlen(tag_obj);
	for (i = 0, j = 0; ptr[j] != ','; ++i, ++j) tmp[i] = ptr[j];
	tmp[i] = 0;
	pt.obj_ra = atof(tmp);
	for (i = 0, ++j; ptr[j] != '>'; ++i, ++j) tmp[i] = ptr[j];
	tmp[i] = 0;
	pt.obj_dc = atof(tmp);

	// sky: 天球位置, ra, dec
	// 从ptr取天球位置
	ptr = strstr(line, tag_sky);
	if (!ptr) return false;
	ptr += strlen(tag_sky);
	for (i = 0, j = 0; ptr[j] != ','; ++i, ++j) tmp[i] = ptr[j];
	tmp[i] = 0;
	pt.sky_ra = atof(tmp);
	for (i = 0, ++j; ptr[j] != '>'; ++i, ++j) tmp[i] = ptr[j];
	tmp[i] = 0;
	pt.sky_dc = atof(tmp);

	return true;
}

/*!
 * @brief 解析一个log文件
 * @note
 * 文件名格式: gtoaes_ccyymmdd.log
 */
void resolve_logfile(const string &pathdir, const string &filename) {
	printf("%s\t\t", filename.c_str());

	int ymd, yy, mm, dd;   // 本地时
	string devid_old("");
	FILE *fpdst = NULL;
	// 从文件名取日期
	sscanf(filename.c_str(), "gtoaes_%d.log", &ymd);
	yy = ymd / 10000;
	dd = ymd % 100;
	mm = (ymd - dd - yy * 10000) / 100;

	// 创建log文件访问环境
	path filepath = pathdir;
	filepath /= filename;
	FILE *fplog = fopen(filepath.c_str(), "r");
	if (!fplog) {
		printf ("failed to access file : %s\n", filename.c_str());
		return;
	}

	char line[200];
	pt_line pt;
	double mjd, lmst, ha_obj, ha_sky;
	double az, el, zobj, zsky;
	double err_ra, err_dc, err_z;

	devls.clear();  // 新的log文件, 强制从新开始判读

	// 遍历行数据
	while (!feof(fplog)) {
		if (fgets(line, 200, fplog) == NULL) continue;
		if (!resolve_line(&line[0], pt)) continue;
		// 计算恒星时与时角
		ats.SetUTC(yy, mm, dd, ((pt.hh * 60 + pt.mm) * 60 + pt.ss) / DAYSEC);
		mjd = ats.ModifiedJulianDay();
		mjd -= fd0; // 本地时转换为UTC
		lmst = ats.LocalMeanSiderealTime(mjd, rlon) * R2D;  // 恒星时: 角度
		ha_obj = lmst - pt.obj_ra;
		if (ha_obj > 180.0) ha_obj -= 360.0;
		else if (ha_obj < -180.0) ha_obj += 360.0;
		ha_sky = lmst - pt.sky_ra;
		if (ha_sky > 180.0) ha_sky -= 360.0;
		else if (ha_sky < -180.0) ha_sky += 360.0;

		// 检查是否新的指向
		vector<last_point>::iterator it;
		int rslt; // rslt == 0: 新的指向; rslt == 2: 新的设备, 新的指向
		for (it = devls.begin(); it != devls.end(); ++it) {
			if ((rslt = it->is_last(pt.devid, pt.obj_ra, pt.obj_dc)) <= 1) break;
		}
		if (rslt == 2 || it == devls.end()) {// 未归档的设备编号
			// 归档设备
			last_point devnew;
			devnew.devid = pt.devid;
			devnew.ra    = pt.obj_ra;
			devnew.dc    = pt.obj_dc;
			devls.push_back(devnew);
		}

		if (rslt == 0 || rslt == 2) {
			// 打开对应的目标文件
			if (devid_old != pt.devid) {
				if (fpdst) fclose(fpdst);

				string pathdst = "err_" + pt.devid + ".txt";   // 输出文件名: "err_" + devid + ".txt"
				fpdst = fopen(pathdst.c_str(), "a+");
			}
			// 输出结果1
			ats.Eq2Horizon(ha_obj * D2R, pt.obj_dc * D2R, az, el);
			zobj = 90.0 - el * R2D;
			ats.Eq2Horizon(ha_sky * D2R, pt.sky_dc * D2R, az, el);
			zsky = 90.0 - el * R2D;
			err_z = (zobj - zsky) * 3600.0;

			err_ra = pt.obj_ra - pt.sky_ra;
			err_dc = (pt.obj_dc - pt.sky_dc) * 3600.0;
			if (err_ra > 180.0) err_ra -= 360.0;
			else if (err_ra < -180.0) err_ra += 360.0;
			err_ra *= 3600.0;
			//                         本地时                   目标位置         天球位置     指向偏差            天顶距
			//               ---------------------------   ----------------- ----------- ----------- ------------------
			//               年  月   日    时   分    秒    赤经    赤纬  时角  赤经   赤纬 赤经   赤纬     目标   天球   偏差
			fprintf (fpdst, "%d %02d %02d %02d %02d %02d  %8.4f %8.4f %9.4f %8.4f %8.4f %7.0f %7.0f  %4.1f %4.1f %6.0f\r\n",
					yy, mm, dd, pt.hh, pt.mm, pt.ss,
					pt.obj_ra, pt.obj_dc, ha_obj,
					pt.sky_ra, pt.sky_dc,
					err_ra, err_dc,
					zobj, zsky, err_z);

			// 输出结果2: 按赤纬轴分别输出
			char dcname[50];
			FILE *fpdc;

			sprintf(dcname, "err_%s%+03d.txt", pt.devid.c_str(), int(pt.obj_dc));
			fpdc = fopen(dcname, "a+");
			fprintf (fpdc, "%9.4f %7.0f %7.0f\r\n",
					ha_obj, err_ra * cos(pt.obj_dc * D2R), err_dc);

			fclose(fpdc);
		}
	}

	fclose(fplog);
	if (fpdst) fclose(fpdst);

	printf("\n");
}

int main(int argc, char **argv) {
	path dirlog;
	if (argc == 1) dirlog = "."; // 当前目录
	else dirlog = argv[1];

	ats.SetSite(lon, lat, alt, tz);

	string extlog(".log");
	// 遍历目录下所有.log文件
	for (directory_iterator x = directory_iterator(dirlog); x != directory_iterator(); ++x) {
		if (x->path().filename().extension().string() == extlog) {
			resolve_logfile(dirlog.string(), x->path().filename().string());
		}
	}

	return 0;
}
