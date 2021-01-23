/*******************************************************************************
 *
 * Replacement open-source erase/write RAM routines for STM8 ROM bootloader
 * https://github.com/basilhussain/stm8-bootloader-erase-write
 *
 * Copyright 2021 Basil Hussain
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

unsigned char bin_erase_write_ver_128k_2_4_ihx[] = {
  0x3a, 0x32, 0x30, 0x30, 0x30, 0x41, 0x30, 0x30, 0x30, 0x38, 0x39, 0x37,
  0x32, 0x35, 0x46, 0x30, 0x30, 0x39, 0x42, 0x37, 0x32, 0x30, 0x38, 0x30,
  0x30, 0x38, 0x45, 0x30, 0x32, 0x32, 0x30, 0x30, 0x33, 0x43, 0x44, 0x30,
  0x31, 0x30, 0x31, 0x30, 0x46, 0x30, 0x31, 0x37, 0x42, 0x30, 0x31, 0x43,
  0x31, 0x30, 0x30, 0x38, 0x38, 0x32, 0x32, 0x33, 0x41, 0x35, 0x46, 0x37,
  0x42, 0x30, 0x31, 0x39, 0x37, 0x31, 0x43, 0x30, 0x30, 0x30, 0x30, 0x46,
  0x36, 0x39, 0x41, 0x0a, 0x3a, 0x32, 0x30, 0x30, 0x30, 0x43, 0x30, 0x30,
  0x30, 0x38, 0x38, 0x43, 0x44, 0x30, 0x31, 0x30, 0x38, 0x38, 0x34, 0x30,
  0x46, 0x30, 0x32, 0x37, 0x42, 0x30, 0x32, 0x41, 0x31, 0x30, 0x38, 0x32,
  0x34, 0x32, 0x31, 0x43, 0x44, 0x36, 0x30, 0x38, 0x41, 0x33, 0x35, 0x32,
  0x30, 0x35, 0x30, 0x35, 0x42, 0x33, 0x35, 0x44, 0x46, 0x35, 0x30, 0x35,
  0x43, 0x43, 0x44, 0x30, 0x30, 0x46, 0x34, 0x43, 0x44, 0x30, 0x33, 0x30,
  0x30, 0x43, 0x37, 0x30, 0x30, 0x46, 0x33, 0x0a, 0x3a, 0x32, 0x30, 0x30,
  0x30, 0x45, 0x30, 0x30, 0x30, 0x39, 0x42, 0x43, 0x45, 0x30, 0x30, 0x38,
  0x42, 0x31, 0x43, 0x30, 0x30, 0x38, 0x30, 0x43, 0x46, 0x30, 0x30, 0x38,
  0x42, 0x30, 0x43, 0x30, 0x32, 0x32, 0x30, 0x44, 0x39, 0x30, 0x43, 0x30,
  0x31, 0x32, 0x30, 0x42, 0x46, 0x38, 0x35, 0x38, 0x31, 0x35, 0x46, 0x34,
  0x46, 0x39, 0x32, 0x41, 0x37, 0x30, 0x30, 0x38, 0x41, 0x35, 0x43, 0x41,
  0x33, 0x30, 0x30, 0x30, 0x34, 0x32, 0x35, 0x46, 0x36, 0x38, 0x45, 0x0a,
  0x3a, 0x32, 0x30, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x38, 0x31, 0x34,
  0x42, 0x38, 0x31, 0x43, 0x44, 0x30, 0x33, 0x31, 0x30, 0x38, 0x34, 0x38,
  0x31, 0x41, 0x45, 0x30, 0x30, 0x38, 0x41, 0x37, 0x46, 0x37, 0x42, 0x30,
  0x33, 0x41, 0x31, 0x38, 0x31, 0x32, 0x36, 0x30, 0x39, 0x33, 0x35, 0x30,
  0x30, 0x30, 0x30, 0x38, 0x43, 0x33, 0x35, 0x34, 0x34, 0x30, 0x30, 0x38,
  0x42, 0x38, 0x31, 0x37, 0x42, 0x30, 0x33, 0x41, 0x31, 0x38, 0x30, 0x32,
  0x36, 0x31, 0x43, 0x0a, 0x3a, 0x32, 0x30, 0x30, 0x31, 0x32, 0x30, 0x30,
  0x30, 0x30, 0x39, 0x33, 0x35, 0x30, 0x30, 0x30, 0x30, 0x38, 0x43, 0x33,
  0x35, 0x34, 0x30, 0x30, 0x30, 0x38, 0x42, 0x38, 0x31, 0x37, 0x42, 0x30,
  0x33, 0x41, 0x31, 0x36, 0x30, 0x32, 0x35, 0x30, 0x35, 0x41, 0x36, 0x30,
  0x32, 0x46, 0x37, 0x32, 0x30, 0x30, 0x39, 0x37, 0x42, 0x30, 0x33, 0x41,
  0x31, 0x32, 0x30, 0x32, 0x35, 0x30, 0x33, 0x41, 0x36, 0x30, 0x31, 0x46,
  0x37, 0x37, 0x42, 0x30, 0x33, 0x38, 0x30, 0x0a, 0x3a, 0x30, 0x43, 0x30,
  0x31, 0x34, 0x30, 0x30, 0x30, 0x39, 0x37, 0x34, 0x46, 0x30, 0x32, 0x35,
  0x38, 0x35, 0x38, 0x31, 0x43, 0x38, 0x30, 0x30, 0x30, 0x43, 0x46, 0x30,
  0x30, 0x38, 0x42, 0x38, 0x31, 0x41, 0x34, 0x0a, 0x3a, 0x32, 0x30, 0x30,
  0x31, 0x38, 0x30, 0x30, 0x30, 0x38, 0x38, 0x37, 0x32, 0x35, 0x46, 0x30,
  0x30, 0x39, 0x43, 0x37, 0x32, 0x30, 0x43, 0x30, 0x30, 0x38, 0x45, 0x30,
  0x32, 0x32, 0x30, 0x31, 0x39, 0x37, 0x32, 0x30, 0x30, 0x30, 0x30, 0x39,
  0x38, 0x30, 0x32, 0x32, 0x30, 0x30, 0x41, 0x33, 0x35, 0x38, 0x31, 0x35,
  0x30, 0x35, 0x42, 0x33, 0x35, 0x37, 0x45, 0x35, 0x30, 0x35, 0x43, 0x32,
  0x30, 0x30, 0x38, 0x33, 0x35, 0x30, 0x31, 0x35, 0x30, 0x37, 0x46, 0x0a,
  0x3a, 0x32, 0x30, 0x30, 0x31, 0x41, 0x30, 0x30, 0x30, 0x35, 0x42, 0x33,
  0x35, 0x46, 0x45, 0x35, 0x30, 0x35, 0x43, 0x37, 0x32, 0x35, 0x46, 0x30,
  0x30, 0x39, 0x38, 0x30, 0x46, 0x30, 0x31, 0x37, 0x42, 0x30, 0x31, 0x43,
  0x31, 0x30, 0x30, 0x38, 0x38, 0x32, 0x32, 0x32, 0x41, 0x43, 0x44, 0x36,
  0x30, 0x38, 0x41, 0x39, 0x30, 0x35, 0x46, 0x37, 0x42, 0x30, 0x31, 0x39,
  0x30, 0x39, 0x37, 0x35, 0x46, 0x37, 0x42, 0x30, 0x31, 0x39, 0x37, 0x31,
  0x43, 0x41, 0x34, 0x0a, 0x3a, 0x32, 0x30, 0x30, 0x31, 0x43, 0x30, 0x30,
  0x30, 0x30, 0x30, 0x30, 0x30, 0x46, 0x36, 0x39, 0x30, 0x38, 0x39, 0x38,
  0x38, 0x43, 0x44, 0x30, 0x31, 0x45, 0x42, 0x35, 0x42, 0x30, 0x33, 0x37,
  0x32, 0x30, 0x44, 0x30, 0x30, 0x38, 0x45, 0x30, 0x32, 0x32, 0x30, 0x30,
  0x36, 0x43, 0x44, 0x30, 0x33, 0x30, 0x30, 0x43, 0x37, 0x30, 0x30, 0x39,
  0x43, 0x30, 0x43, 0x30, 0x31, 0x32, 0x30, 0x43, 0x46, 0x37, 0x32, 0x30,
  0x43, 0x30, 0x30, 0x38, 0x45, 0x30, 0x31, 0x0a, 0x3a, 0x31, 0x34, 0x30,
  0x31, 0x45, 0x30, 0x30, 0x30, 0x30, 0x32, 0x32, 0x30, 0x30, 0x36, 0x43,
  0x44, 0x30, 0x33, 0x30, 0x30, 0x43, 0x37, 0x30, 0x30, 0x39, 0x43, 0x38,
  0x34, 0x38, 0x31, 0x37, 0x42, 0x30, 0x33, 0x31, 0x45, 0x30, 0x34, 0x39,
  0x32, 0x41, 0x37, 0x30, 0x30, 0x38, 0x41, 0x38, 0x31, 0x43, 0x37, 0x0a,
  0x3a, 0x32, 0x30, 0x30, 0x33, 0x30, 0x30, 0x30, 0x30, 0x43, 0x36, 0x35,
  0x30, 0x35, 0x46, 0x41, 0x35, 0x30, 0x31, 0x32, 0x37, 0x30, 0x33, 0x41,
  0x36, 0x30, 0x31, 0x38, 0x31, 0x41, 0x35, 0x30, 0x34, 0x32, 0x37, 0x46,
  0x32, 0x34, 0x46, 0x38, 0x31, 0x34, 0x46, 0x31, 0x31, 0x30, 0x33, 0x32,
  0x32, 0x30, 0x45, 0x38, 0x38, 0x43, 0x44, 0x36, 0x30, 0x38, 0x41, 0x38,
  0x34, 0x35, 0x46, 0x39, 0x37, 0x31, 0x43, 0x30, 0x30, 0x30, 0x30, 0x46,
  0x37, 0x37, 0x46, 0x0a, 0x3a, 0x30, 0x39, 0x30, 0x33, 0x32, 0x30, 0x30,
  0x30, 0x34, 0x43, 0x32, 0x30, 0x45, 0x45, 0x37, 0x42, 0x30, 0x33, 0x43,
  0x37, 0x30, 0x30, 0x38, 0x38, 0x38, 0x31, 0x32, 0x43, 0x0a, 0x3a, 0x30,
  0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x46, 0x46, 0x0a
};
unsigned int bin_erase_write_ver_128k_2_4_ihx_len = 814;
