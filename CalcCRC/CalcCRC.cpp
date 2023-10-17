// CalcCRC.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <cstdint>

static uint16_t ComputeCRC(const uint8_t* pData, const int nLength, const uint16_t nInitialValue)
{
    unsigned CRC = nInitialValue;
//    constexpr unsigned Polynomial = 0xa001;
    constexpr unsigned Polynomial = 0x8408;
    for (int i = 0; i < nLength; ++i)
    {
        CRC ^= pData[i];

        for (int j = 0; j < 8; ++j)
        {
            bool Carry = (CRC & 1) != 0;
            CRC >>= 1;
            if (Carry)
            {
                CRC ^= Polynomial;
            }
        }
    }

    return static_cast<uint16_t>(CRC);
}


int main()
{
    FILE *file=fopen("C:\\Src\\Clients\\Innovent Integrated\\Video\\raw_0.hex","rb");
    if (!file)
    {
        std::cerr << "Failed to open file\n";
        return -1;
    }

    fseek(file, 0, SEEK_END);
    const long len = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* buffer = new uint8_t[len];

    fread(buffer, 1, len, file);

    const uint16_t expectedCRC = (buffer[len - 2] << 8) | buffer[len - 1];
//    constexpr int InitialValue = 0x8408;
//    constexpr int InitialValue = 0x0884;
//    constexpr int InitialValue = 0;

    for (unsigned InitialValue = 0; InitialValue < 0x10000; ++InitialValue)
    {
        const uint16_t crc = ComputeCRC(buffer + 4, len - 6, InitialValue);
        if (crc == expectedCRC)
        {
            std::cout << "Initial =" << std::hex << InitialValue << "\n";
            std::cout << "expected CRC=" << std::hex << expectedCRC << "\n";
            std::cout << "computed CRC=" << std::hex << crc << "\n";
        }
    }

    delete[] buffer;

    fclose(file);
    return 0;
}
