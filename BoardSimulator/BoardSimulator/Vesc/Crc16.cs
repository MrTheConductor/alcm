using System;

namespace BoardSimulator.Vesc
{
    /// <summary>
    /// CRC-16 CCITT implementation matching the ALCM firmware
    /// </summary>
    public static class Crc16
    {
        private const ushort Polynomial = 0x1021;
        private const ushort InitialValue = 0x0000;

        public static ushort Calculate(byte[] data, int offset, int length)
        {
            ushort crc = InitialValue;

            for (int i = 0; i < length; i++)
            {
                crc ^= (ushort)(data[offset + i] << 8);

                for (int bit = 0; bit < 8; bit++)
                {
                    if ((crc & 0x8000) != 0)
                    {
                        crc = (ushort)((crc << 1) ^ Polynomial);
                    }
                    else
                    {
                        crc <<= 1;
                    }
                }
            }

            return crc;
        }

        public static ushort Calculate(byte[] data)
        {
            return Calculate(data, 0, data.Length);
        }
    }
}
