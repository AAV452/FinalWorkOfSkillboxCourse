#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <thread>
#include <mutex>
//#include <cstddef> //для std::size_t
#include "invertedIndex.h"


//метод обновления или заполнения docs - базы содержимого документов, по которой будем совершать поиск:
//на входе вектор путей к файлам - ConverterJSON.Config.filesPaths:
void InvertedIndex::updateDocumentsBase(std::vector<std::string> inputDocs)
{
  //очищаем список путей:
  validDocsPaths.clear();
  //теперь очищаем память от вектора:
  //или гарантированно освобождаем память временным пустым вектором (для версий до С++11):
  //std::vector<std::string>().swap(validDocsPaths);
  //или запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11):
  validDocsPaths.shrink_to_fit();

  //1. необходимо отобрать лишь валидные пути, чтобы по индексу пути в векторе связывать другие контейнеры:
  for ( auto pathFile : inputDocs )
  {
    std::ifstream iTempFile(pathFile);
    if ( !iTempFile.is_open() )
      std::cout << "Файл \"" << pathFile << "\" не открывается или отсутствует!\n";
    else
      validDocsPaths.push_back(pathFile);

    iTempFile.close();
  }

  //2. запускаем для каждого файла поток, в котором:
  //         заносим содержимое файла в вектор текстов; 
  //         разбиваем текст файла на слова;
  //         заполняем частотный словарь.
  //предварительно очищаем вектор текстов документов и частотный словарь:
  docs.clear(); //удаляет все элементы, но не освобождает память
  //очищаем память:
  //гарантированно освобождаем память временным пустым вектором (для версий до С++11)
  //std::vector<std::string>().swap(docs);
  docs.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
  docs.resize(validDocsPaths.size()); //количество текстов равно количеству валидных файлов
  
  freq_dictionary.clear(); //очищаем частотный словарь

  //запускаем цикл потоков для обработки каждого валидного пути:
  std::vector<std::thread> threads;
  std::mutex mtxDocs, mtxFreq;

  for ( int i = 0; i < validDocsPaths.size(); i++ )
  {
    //для каждого пути файла будет создан поток, в котором текст файла добавится в список текстов документов, в самом тексте
    //пошагово будет: искаться слвово, осуществлятся его проверка на уникальность и запись в частотный словарь.
    //метод вектора emplace_back создаёт элемент непосредственно внутри памяти вектора, используя переданные
    //аргументы (аргументами потока будут функция и её параметры), исключая лишнюю операцию копирования или
    //перемещения (в отличие от push_back). Т.е. внутри emplace_back будет запущен с параметрами конструктор потока:
    //threads.emplace_back(&, i);
    threads.emplace_back([&, i]()
    { //[&] - открывает видимость всех переменных из основного контекста в анонимный

      //************************************ СЧИТЫВАЕМ ТЕКСТ ФАЙЛА ************************************
      std::ifstream iTextFile(validDocsPaths[i]); //открываем файл

      //чтение строки str из входного потока is, пока не будет достигнут конец файла или символ-разделитель delim
      //(по умолчанию delim = '\n'):  std::getline(std::istream &is, std::string &str, char delim);
      //чтение всех строк файла, пока не достигнут конец файла:
      std::string strLine = "",
                  text = "";

      while ( std::getline(iTextFile, strLine) )
        text += ( text == "" ) ? strLine : (" " + strLine); //переход на новую строку заменяем пробелом

      //блокируем все потоки, пока текущий передаёт значение в вектор:
      mtxDocs.lock();
        docs[i] = text; //записываем текст по индексу его файла в validDocsPath
      mtxDocs.unlock();

      iTextFile.close();

      //************************** РАЗБИВАЕМ ТЕКСТ ФАЙЛА НА ОТДЕЛЬНЫЕ СЛОВА **************************
      //********************************* И СЧИТАЕМ УНИКАЛЬНЫЕ СЛОВА *********************************
      std::string word = "";

      bool flag = false; //слово не сформировано

      //идем по тексту:
      for ( int j = 0; j < text.size(); j++ )
      {
        //если символ - цифра, буква..., то начать формировать слово:
        if ( ( text[j] >= '0' && text[j] <= '9' ) ||
             ( text[j] >= 'a' && text[j] <= 'z' ) ||
             ( text[j] >= 'A' && text[j] <= 'Z' ) )
          //начинаем формировать слово из букв/цифр:
        {
          word += text[j];
          flag = false;
        }
        else
        if ( word != "" && !flag) //если символ - не цифра/буква, а слово начато, то слово сформировано
          flag = true;

        if ( word != "" && j == text.size() - 1 ) //если последний символ текста - цифра/буква, то слово сформировано
          flag = true;

        if ( flag )
        { 
          //блокируем потоки, пока текущий работает со словом и с частотным словарём:
          mtxFreq.lock();
            //проверяем уникальность слова в тексте: 
            auto it = freq_dictionary.find(word);
            if ( it == freq_dictionary.end() ) //результат поиска сравниваем с итератором end
            {
              //такого слова нет в словаре, значит - добавим его в словарь:
              std::pair<std::string, std::vector<Entry>> pairTemp;
              pairTemp.first = word;
              //Entry ent = {static_cast<std::size_t>(i), 1};
              pairTemp.second.push_back( {static_cast<std::size_t>(i), 1} ); //static_cast, чтобы из int в size_t
              freq_dictionary.insert(pairTemp);
            }
            else //иначе, слово есть в словаре, тогда увеличим его количество на 1 в документе c индексом i:
            {
              bool findDoc = false;
              //ищем в векторе нужный документ:
              for ( auto &entry : it->second )
                if ( entry.doc_id == i )
                {
                  entry.count++;
                  findDoc = true;
                  break; //for
                }
              
              if ( !findDoc ) //документ i ещё не внесён в вектор
              {
                //Entry ent = {i, 1};
                it->second.push_back( {static_cast<std::size_t>(i), 1} ); //static_cast, чтобы из int в size_t
              }
            }

          mtxFreq.unlock();

          word = ""; //готовимся к следующему слову
          flag = false;
        }
      }
    }); //лямбда-функция
  }
  
  //запускаем цикл ожидания завершения всех потоков - синхронизацию:
  for ( auto &t : threads )
    t.join();

  //т.к. потоки отработали параллельно, то не ясно, в какой последовательности потоки заполнили частотный словарь.
  //т.о. нужно отсортировать "вхождения слов в тексты" в векторах в порядке, согласно увеличения индекса текстовых файлов:
  //сортировку вхождения каждого слова делать будем в новом потоке:

  //1. очищаем вектор потоков:
  threads.clear(); //удаляет все элементы, но не освобождает память
  //очищаем память:
  threads.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)

  for ( auto &wordFreq : freq_dictionary ) //идём по каждому слову частотного словаря
  {
    threads.emplace_back([&wordFreq]() 
    { //[&wordFreq] - открываем в лямбде видимость пары: слово-'вектор_вхождений_слова_в_документы'
      //2. сортируем пузырьком 'вектор вхождения слова в тексты документов' по возрастанию индекса документов:
      for ( int i = 0; i < wordFreq.second.size(); i++ )
      {
        int min = wordFreq.second[i].doc_id;

        for ( int j = i + 1; j < wordFreq.second.size(); j++ )
          if ( wordFreq.second[j].doc_id < min )
          {
            min = wordFreq.second[j].doc_id;
            Entry tempMin_id;
            tempMin_id = wordFreq.second[j];
            wordFreq.second[j] = wordFreq.second[i];
            wordFreq.second[i] = tempMin_id;
          }
      }
    });
  }

  //запускаем цикл ожидания завершения всех потоков - синхронизацию:
  for ( auto &t : threads )
    t.join();
}


//Метод определяет количество вхождений слова word в загруженной базе документов.
//(word - слово, частоту вхождений которого необходимо определить):
std::vector<Entry> InvertedIndex::getWordCount(const std::string &word)
{
  auto it = freq_dictionary.find(word); //итератор на найденный вектор
  if ( it == freq_dictionary.end() )
  {
    std::cout << "Слова \"" << word << "\" нет ни в одном файле!\n";
    return {};
  }
  else
    return it->second;
}



