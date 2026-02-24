#pragma once //прагма - команда прекомпилятора - исключает повторение и пересечение вызовов
             //данного .h-файла в других срр-файлах и разных сценариях проекта

#include <map>

struct Entry
{
    //Size_t — тип, используемый при задании размеров и индексации коллекций, обычно — это беззнаковый int, но
    //правильнее писать именно size_t:
    size_t doc_id, //номер элемента в векторе InvertedIndex.docs (см. ниже), по которому хранится текст;
           count; //число, которое обозначает, сколько раз ключевое слово встретилось в документе doc_id

    // Данный оператор необходим для проведения тестовых сценариев:
    //Перегружаем оператор сравнения ==, позволяя быстро и безопасно сравнивать два экземпляра структуры
    //напрямую через ==. Возвращает true, если структуры равны, или false - если нет:
    bool operator == (const Entry &other) const
    {
        return ( doc_id == other.doc_id && count == other.count );
    }
};

class InvertedIndex
{
private:
    std::vector<std::string> docs; //вектор текстов файлов из вектора ConverterJSON.config.filesPaths

    std::vector<std::string> validDocsPaths; //валидные файлы

    std::map<std::string, std::vector<Entry>> freq_dictionary; //частотный словарь: в коллекции ключом служат слова 
                                    //из загруженных текстов, а значением — вектор структур с полями doc_id и count
public:
    InvertedIndex() = default;

    ~InvertedIndex() { /*std::cout << "Класс InvertedIndex отработал!!!";*/ };

    //метод обновления или заполнения docs - базы содержимого документов, по которой будем совершать поиск:
    //на входе - вектор путей к поисковым документам - ConverterJSON.config.filesPaths:
    //(считаю, лучше использовать список путей вместо содержимого файлов, потому что, запуская метод обновления,
    //мы напрямую получаем актуальное содержимое, возможно, измененных (удалённых) файлов с последнего обновления словаря.
    //Иначе пришлось бы дополнительно предварительно получать содержимое файлов методом класса converterJSON)
    void updateDocumentsBase(std::vector<std::string> inputDocs);

    //метод определяет количество вхождений слова word в загруженной базе документов.
    //word - слово, частоту вхождений которого необходимо определить.
    std::vector<Entry> getWordCount(const std::string &word);

    std::vector<std::string> getTextsDocs()
    {
      return docs;
    }

    std::vector<std::string> getValidDocsPaths()
    {
      return validDocsPaths;
    }

    std::map<std::string, std::vector<Entry>> getFreq_dictionary()
    {
      return freq_dictionary;
    }

};
