#include <bitset>
#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <string>

class GF2m {
private:
    std::bitset<173> value;
    static const int m = 173;

public:
    GF2m() = default;

    GF2m(const std::bitset<m>& val) : value(val) {}

    GF2m(const std::string& str, bool isHex = 0) {
        *this = isHex ? fromHex(str) : fromString(str);            
    }

    GF2m add(const GF2m& other) const {
        return GF2m(value ^ other.value);
    }

    GF2m multiply(const GF2m& other) const {
        std::bitset<2 * m - 1> result;
        for (int i = 0; i < m; ++i) {
            if (other.value.test(i)) {
                for (int j = 0; j < m; ++j) {
                    if (value.test(j)) {
                        result.flip(i + j);
                    }
                }
            }
        }
        return GF2m(modReduce(result));
    }

    static std::bitset<m> modReduce(const std::bitset<2 * m - 1>& poly) {
        std::bitset<2 * m - 1> mod_poly;
        //x^173+x^10+x^2+x+1
        mod_poly.set(173);
        mod_poly.set(10);
        mod_poly.set(2);
        mod_poly.set(1);
        mod_poly.set(0);

        std::bitset<2 * m - 1> result = poly;
        for (int i = 2 * m - 2; i >= m; --i) {
            if (result.test(i)) {
                result ^= mod_poly << (i - m);
            }
        }

        // Створення нового bitset для молодших m бітів
        std::bitset<m> reduced_result;
        for (int i = 0; i < m; ++i) {
            reduced_result[i] = result[i];
        }
        return reduced_result;
    }

    GF2m square() const {
        std::bitset<2 * m - 1> squareTemp; 
        for (int i = 0; i < m; ++i) {
            if (value.test(i)) {
                squareTemp.set(2 * i);
            }
        }
        return GF2m(modReduce(squareTemp));
    }

    GF2m pow(const GF2m& power) const {
        GF2m result(std::bitset<m>(1)); // Нейтральний елемент за множенням (1 у GF(2^m))
        GF2m base = *this;

        for (int i = 0; i < m; ++i) {
            if (power.value.test(i)) {
                result = result.multiply(base);
            }
            base = base.square();
        }
        return result;
    }

    int trace() const {
        GF2m temp = *this;
        GF2m trace = std::bitset<m>(0);
        for (int i = 0; i < m; ++i) {
            trace = trace.add(temp);
            temp = temp.square();
        }
        // Оскільки результат сліду - це 0 або 1, повертаємо молодший біт
        return trace.value[0];
    }

    GF2m inverse() const {
        if (this->value.none()) {
            throw std::runtime_error("Неможливо знайти обернений елемент до нуля");
        }

        GF2m result = GF2m(*this);
        GF2m temp = GF2m(*this);
        for (int i = 0; i < m - 2; ++i) {
            temp = temp.square();
            result = result.multiply(temp);                           
        }

        result = result.square();
        return result;
    }

    std::string toString() const {
        std::string str;
        for (int i = m - 1; i >= 0; --i) {
            str += (value.test(i) ? '1' : '0');
        }
        return str;
    }

    static GF2m fromString(const std::string& str) {
        std::string result = str;
        if (str.size() > m) {
            throw std::invalid_argument("Рядок має неправильну довжину");
        }
        if (str.size() < m) {
            // Доповнюємо рядок нулями зліва
            result = std::string(m - str.size(), '0') + str;
        }
        for (char c : str) {
            if (c != '0' && c != '1') {
                throw std::invalid_argument("Неправильний символ у рядку");
            }
        }
        return GF2m(std::bitset<m>(result));
    }

    std::string toHex() const {
        std::string hexStr;

        for (int i = 0; i < m; i += 4) {
            int hexValue = 0;
            for (int j = 0; j < 4 && (i + j) < m; ++j) {
                if (value.test(i + j)) {
                    hexValue |= 1 << j;
                }
            }

            char hexChar = hexValue < 10 ? '0' + hexValue : 'A' + hexValue - 10;
            hexStr = hexChar + hexStr;
        }

        // Видалення лідуючих нулів
        size_t startPos = hexStr.find_first_not_of('0');
        if (startPos != std::string::npos) {
            hexStr = hexStr.substr(startPos);
        }
        else {
            hexStr = "0"; // Всі біти були нулями
        }

        return hexStr;
    }

    static GF2m fromHex(const std::string& hexStr) {
        std::string binStr;
        for (char hexChar : hexStr) {
            int hexValue;
            if (hexChar >= '0' && hexChar <= '9') {
                hexValue = hexChar - '0';
            }
            else if (hexChar >= 'A' && hexChar <= 'F') {
                hexValue = hexChar - 'A' + 10;
            }
            else if (hexChar >= 'a' && hexChar <= 'f') {
                hexValue = hexChar - 'a' + 10;
            }
            else {
                hexValue = -1; // Неправильний символ
            }

            if (hexValue == -1) {
                throw std::invalid_argument("Неправильний символ у шістнадцятковому рядку");
            }
            for (int i = 3; i >= 0; --i) {
                binStr += (hexValue & (1 << i)) ? '1' : '0';
            }
        }
        // Обрізаємо старші нулі, якщо це необхідно
        auto firstNonZero = binStr.find_first_not_of('0');
        if (firstNonZero != std::string::npos) {
            binStr = binStr.substr(firstNonZero);
        }
        if (binStr.size() > m) {
            throw std::invalid_argument("Шістнадцятковий рядок занадто довгий");
        }
        // Доповнюємо рядок нулями зліва, якщо він коротший за m
        if (binStr.size() < m) {
            binStr = std::string(m - binStr.size(), '0') + binStr;
        }
        return GF2m(std::bitset<m>(binStr));
    }

    void print() const {
        for (int i = m - 1; i >= 0; --i) {
            std::cout << value.test(i);
        }
        std::cout << std::endl;
    }

    GF2m& operator=(const GF2m& other) {
        if (this != &other) {  
            this->value = other.value;
        }
        return *this;
    }

    bool operator==(const GF2m& other) const {
        return value == other.value;
    }

    GF2m operator+(const GF2m& other) const {
        GF2m result;
        result.value = this->value ^ other.value;
        return result;
    }

    GF2m operator*(const GF2m& other) const {
        return this->multiply(other);
    }
};



void testAddition() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    GF2m B1("01001001111011010100111010001010100001100000000110011011100010110000011100001000101011011110101001010011101111000110011100100001101101110000111000101010011000111011110011111");
    assert(A1 + B1 == GF2m("00011001101100010101111111000000011011100100111010111011000100110100001001001001111101100000100100101110000010100011100101010111001110001011111001010111010111100000100001100"));
    
    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    GF2m B2("01101110101010111011100111101000101110110110010011100000011100011100111110010000000011111001000001001011000000111000000011001110011110101010001100110011100000111011011101011");
    assert(A2 + B2 == GF2m("01000000000101011001110001100000100011011011110100101101001111100000100010001011111011011001010100100010010010100110101100011011001110000111101001000001011011110001000101111"));
    
    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    GF2m B3("0D5026BF220F27A2D765193E6C14502E37F19293A040", 1);
    assert(A3 + B3 == GF2m("07B93B08D9DECC0EB17A7DB6A033A226F540A4DA9221", 1));
    
    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    GF2m B4("08A7A2AAFB1EE180992EF7BD70265A48086F4B84842D", 1);
    assert(A4 + B4 == GF2m("1FBF8DEA9E54C2E8B62E34C47B083D5CC6F8B3803B99", 1));
    
    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    GF2m B5("ABCDFAFACBACFACBACFACBACFACB", 1);
    assert(A5 + B5 == GF2m("ABCDE5171170467110467073724073724", 1));
}
void testMultiplication() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    GF2m B1("01001001111011010100111010001010100001100000000110011011100010110000011100001000101011011110101001010011101111000110011100100001101101110000111000101010011000111011110011111");
    assert(A1 * B1 == GF2m("101101000110100000010111111010111011001110100001000011011110111011000011001010110001010101101100111101001110111011011000001010001000001111001111000111011011101110100110100"));
    
    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    GF2m B2("01101110101010111011100111101000101110110110010011100000011100011100111110010000000011111001000001001011000000111000000011001110011110101010001100110011100000111011011101011");
    assert(A2 * B2 == GF2m("11010000000001000000100111010101100110110110001100010010110010010010111000011101011110111100101010101010110000100000010101100011000000100100101001010101010000010101110011000"));
    
    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    GF2m B3("0D5026BF220F27A2D765193E6C14502E37F19293A040", 1); 
    assert(A3 * B3 == GF2m("1ad6f26d48849e96d7cb5852de86c4a425b698a35b3e", 1));
    
    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    GF2m B4("08A7A2AAFB1EE180992EF7BD70265A48086F4B84842D", 1);
    assert(A4 * B4 == GF2m("e6230332b928e7dcb7432bbb23d756aec0126d78a26", 1));

    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    GF2m B5("ABCDFAFACBACFACBACFACBACFACB", 1);
    assert(A5 * B5 == GF2m("167c015560cf59fec6516c3105572f4254367b6aa1b9", 1));
}
void testTrace() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    assert(A1.trace() == 1);

    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    assert(A2.trace() == 0);

    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    assert(A3.trace() == 1);

    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    assert(A4.trace() == 1);

    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    assert(A5.trace() == 1);
}
void testSquare() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    assert(A1.square() == GF2m("11011101110001011001010000011110001010111110010011100010101001100010111100100011101011011011000110110101101001001100010000101010000101001001110101011100111010001111000000001"));

    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    assert(A2.square() == GF2m("11001100101110101010100000110100011110110111111111101011010111001110000000001010100100011000011101111111110111100000011001001010011011101110010000011011000010100000111100111"));

    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    assert(A3.square() == GF2m("04251E42190124F9FCB5C2B9835DEA15F30B92DC6E53", 1));

    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    assert(A4.square() == GF2m("0E3136CF4B0F969440A35E3403463BCA4D484F5C32A4", 1));

    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    assert(A5.square() == GF2m("15544450455155544458C2018FF66B909A018F5611E3", 1));
}
void testPow() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    GF2m B1("01001001111011010100111010001010100001100000000110011011100010110000011100001000101011011110101001010011101111000110011100100001101101110000111000101010011000111011110011111");
    assert(A1.pow(B1) == GF2m("11010001100110011001010110111001111110111010000111110100110000100010010010101011011001101010010110111101010100101010011101101110000110011110010000101110001101000011001111011"));

    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    GF2m B2("01101110101010111011100111101000101110110110010011100000011100011100111110010000000011111001000001001011000000111000000011001110011110101010001100110011100000111011011101011");
    assert(A2.pow(B2) == GF2m("01010100001010100101100111010010001110001010000001111100101110000100011110010101000101101011101110000011110000010010110100100101000110010011111100000001000011000011000110011"));

    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    GF2m B3("0D5026BF220F27A2D765193E6C14502E37F19293A040", 1);
    assert(A3.pow(B3) == GF2m("078EFE0F485C47EEBF4BAC5D5372EC334D2E8EDC826C", 1));

    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    GF2m B4("08A7A2AAFB1EE180992EF7BD70265A48086F4B84842D", 1);
    assert(A4.pow(B4) == GF2m("044F59BB04C420D95A3625E96ED50DB507D86818EE12", 1));

    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    GF2m B5("ABCDFAFACBACFACBACFACBACFACB", 1);
    assert(A5.pow(B5) == GF2m("00887DC043F22459BAD991EFEB615E4656314029EFC6", 1));
}
void testInverse() {
    GF2m A1("01010000010111000001000101001010111010000100111100100000100110000100010101000001010110111110001101111101101101100101111001110110100011111011000001111101001111011011010010011");
    assert(A1.inverse() == GF2m("01001110110111101000110010011001011101011110100001010111011001110111101001011000010100110010100000111111101001001010010000110010111010000100010001111100100010101101010001100"));

    GF2m A2("00101110101111100010010110001000001101101101100111001101010011111100011100011011111000100000010101101001010010011110101111010101010000101101100101110010111011001010011000100");
    assert(A2.inverse() == GF2m("10011001110100100111101000000111011011011000001111101010110100111010011001011000110010011101010111010111010110010011011010001010111101100000001001001100011010011111101111100"));

    GF2m A3("0AE91DB7FBD1EBAC661F6488CC27F208C2B136493261", 1);
    assert(A3.inverse() == GF2m("00752E40B2816548D76717E9EB45BF0C26B56DC9ADAD", 1));

    GF2m A4("17182F40654A23682F00C3790B2E6714CE97F804BFB4", 1);
    assert(A4.inverse() == GF2m("0441A08E391F5975D1D560F19BC25289D4032A3CFC39", 1));

    GF2m A5("ABCDEFABCEDFEACBDFEACABCDEFABCDEF", 1);
    assert(A5.inverse() == GF2m("1E6BCB5756F3B08AAF8A1F88E415A6FBBD28F17B506D", 1));
}
void otherTests() {
    std::bitset<173> bitset;
    bitset.set(); // Встановлює всі біти у 1
    GF2m Max(bitset);
    GF2m a("1E5908188E2D4E112EC2B9F5EBDBE7703651A1A520DC", 1);
    GF2m b("0ACE832553C6A2574E988BD34ED55D7918BEDFC36474", 1);
    GF2m c("008B945DE1FD63598C82DC661E9EB94757153572503E", 1);

    //(a+b)c=c(a+b)=ac+bc
    assert((a + b) * c == a * c + b * c);
    assert((a + b) * c == c * (a + b));
    //a^Max=1
    assert(a.pow(Max) == GF2m("1"));
}
std::string generateRandomNumberString(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1); // generating digits between 0 and 9

    std::string result;
    for (int i = 0; i < length; ++i) {
        result += std::to_string(dis(gen));
    }
    return result;
}
void timeTest() {

    GF2m numbers[20];
    for (int i = 0; i < 20; i++) {
        GF2m temp(generateRandomNumberString(173));
        numbers[i] = temp;
    }
    GF2m a;
    int b;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 101; i++) {
        a = numbers[i % 20] + numbers[(i - 1) % 20];
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for addition: " << duration.count() << " microseconds." << std::endl;


    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 101; i++) {
        a = numbers[i % 20] * numbers[(i - 1) % 20];
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for multiplication: " << duration.count() << " microseconds." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 101; i++) {
        b = numbers[i % 20].trace();
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for trace: " << duration.count() << " microseconds." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 101; i++) {
        a = numbers[i % 20].square();
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for square: " << duration.count() << " microseconds." << std::endl;


    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 2; i++) {
        a = numbers[i % 20].pow(numbers[(i - 1) % 20]);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for pow: " << duration.count() << " microseconds." << std::endl;



    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 101; i++) {
        a = numbers[i % 20].inverse();
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken for inverse: " << duration.count() << " microseconds." << std::endl;


}


int main() {
    
    /*testAddition();
    testMultiplication();
    testTrace();
    testSquare();
    testPow();
    testInverse();
    otherTests();
    std::cout << "Всі тейсти прошйли успішно!\n";
    timeTest();*/
    
    /*GF2m a("14828E13ADCFCC3BF7368BD43DE89041763C1DBFB2DE", 1);
    GF2m b("15EB380F0A6C9DFC8BBF6A6A4811BF9D7C451CC12A4C", 1);
    GF2m c("1311EF56624F81C6C43609B74687D8BAF7E0916BDD1E", 1);

    std::cout << "Сума: " << (a.add(b)).toHex() << std::endl;   
    std::cout << "Добуток: " << (a.multiply(b)).toHex() << std::endl;
    std::cout << "Квадрат a^2: " << (a.square()).toHex() << std::endl;
    std::cout << "Степінь a^c: " << (a.pow(c)).toHex() << std::endl;
    std::cout << "Слід a: " << a.trace() << std::endl;;
    std::cout << "Обернений a^-1: " << a.inverse().toHex() << std::endl;
    std::cout << "a*a^-1: " << a.multiply(a.inverse()).toHex() <<std::endl;*/
    

    return 0;
}
