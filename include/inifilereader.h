#ifndef INIFILEREADER_H
#define INIFILEREADER_H

#include <string>
#include <map>
#include <fstream>

/*!
 * @class IniFileReader
 * @brief: Класс для чтения ini-файла
*/
class IniFileReader {
public:
	IniFileReader(const std::string &fileName);
	~IniFileReader();
	/*!
	* @fn IniFileReader::parse
	* @brief: парсинг файла
	*/
	void parse();
	bool isOpened() const;
	/*!
	* @fn IniFileReader::getConfig
	* @brief: получение всей конфигурации
	* NOTE: имя секции по умолчанию: DEFAULT
	*/
	const std::map <std::string, std::map<std::string, std::string> >& getConfig() const;
	/*!
	* @fn IniFileReader::getSection
	* @brief: получение секции
	*/
	const std::map <std::string, std::string>* getSection(std::string section_name) const;

private:
	std::ifstream file;
	std::map <std::string, std::map<std::string, std::string> > configInfo;
};

#endif // INIFILEREADER_H