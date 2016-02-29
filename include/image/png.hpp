#ifndef YAM_PNG_HPP
#define YAM_PNG_HPP

#include <wheel.h>

#include "../image.h"
#include "../renderer.h"
#include "../../deps/miniz.c"

#define ZLIB_CHUNK 262144

namespace yam
{
   struct PNGChunk
   {
      uint32_t       len;
      char           type[4];
      wcl::buffer_t  data;
      uint32_t crc;
   };

   size_t z_compress(const void* src, size_t len, wcl::buffer_t& destination)
   {
      z_stream stream;

      uint8_t temp_buffer[ZLIB_CHUNK];

      wcl::buffer_t buffer;

      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.next_in = (uint8_t*)src;
      stream.avail_in = len;
      stream.next_out = temp_buffer;
      stream.avail_out = ZLIB_CHUNK;

      deflateInit(&stream, Z_BEST_COMPRESSION);

      while (stream.avail_in != 0)
      {
         if (deflate(&stream, Z_NO_FLUSH) != Z_OK)
         {
            log(ERROR, "zlib error: deflate failed\n");
            return 0;
         }

         if (stream.avail_out == 0)
         {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + ZLIB_CHUNK);
            stream.next_out = temp_buffer;
            stream.avail_out = ZLIB_CHUNK;
         }
      }

      int deflate_result = Z_OK;
      while (deflate_result == Z_OK)
      {
         if (stream.avail_out == 0)
         {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + ZLIB_CHUNK);
            stream.next_out = temp_buffer;
            stream.avail_out = ZLIB_CHUNK;
         }

         deflate_result = deflate(&stream, Z_FINISH);
      }

      if (deflate_result != Z_STREAM_END)
      {
         log(ERROR, "zlib error: incomplete stream\n");
         return 0;
      }

      buffer.insert(buffer.end(), temp_buffer, temp_buffer + ZLIB_CHUNK - stream.avail_out);
      deflateEnd(&stream);

      destination.swap(buffer);

      return destination.size();
   }

   size_t z_uncompress(const void* src, size_t len, wcl::buffer_t& destination)
   {
      z_stream stream;

      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;

      stream.avail_in = stream.total_in = len;
      stream.next_in = (uint8_t*)src;
      stream.avail_out = ZLIB_CHUNK;

      uint8_t temp_buffer[ZLIB_CHUNK];
      wcl::buffer_t buffer;

      int ret;

      if ((ret = inflateInit(&stream)) != Z_OK)
      {
         log(ERROR, "zlib error: inflating failed\n");
         return 0;
      }

      while (ret != Z_STREAM_END)
      {
         stream.next_out = temp_buffer;
         stream.avail_out = ZLIB_CHUNK;

         ret = inflate(&stream, Z_NO_FLUSH);

         buffer.insert(buffer.end(), temp_buffer, temp_buffer + ZLIB_CHUNK - stream.avail_out);
      }

      (void)inflateEnd(&stream);
      destination.swap(buffer);

      return destination.size();
   }

   size_t z_uncompress(const wcl::buffer_t& source, wcl::buffer_t& destination)
   {
      if (source.size() == 0)
         return 0;

      return z_uncompress(&source[0], source.size() - 1, destination);
   }

   /*
      PNG functions
   */
   inline uint32_t read_png(const wcl::buffer_t& data, uint32_t* w = nullptr, uint32_t* h = nullptr,
                     uint32_t* c = nullptr, wcl::buffer_t* target = nullptr,
                     palette_t* palette = nullptr)
   {
      uint32_t nw, nh, nc;
      if (w == nullptr)
         w = &nw;
      if (h == nullptr)
         h = &nh;
      if (c == nullptr)
         c = &nc;

      std::vector<PNGChunk*> chunks;

      wcl::buffer_t& buffer = (wcl::buffer_t&)data;
      buffer.seek(8);

      while(buffer.pos() < buffer.size())
      {
         PNGChunk* next = new PNGChunk;

         if (!buffer.can_read(sizeof(next->len)))
         {
            delete next;
            log(ERROR, "Abrupt end of memory buffer: can't read PNG chunk length\n");
            return WHEEL_UNEXPECTED_END_OF_FILE;
         }

         next->len = buffer.read_le<uint32_t>();

         if (!buffer.can_read(sizeof(next->type)))
         {
            delete next;
            log(ERROR, "Abrupt end of memory buffer: can't read PNG chunk type\n");
         }

         strncpy(next->type, (const char*)(&buffer[0] + buffer.pos()), 4);
         wcl::string s(next->type, 4);
         buffer.seek(buffer.pos() + 4);

         if (!buffer.can_read(next->len))
         {
            delete next;
            log(ERROR, "Abrupt end of memory buffer: can't read PNG chunk data\n");
            return WHEEL_UNEXPECTED_END_OF_FILE;
         }

         // TODO: Double-check this.
         next->data.resize(next->len);
         memcpy(&(next->data[0]), (&buffer[0] + buffer.pos()), next->len);
         buffer.seek(buffer.pos() + next->len);

         if (!buffer.can_read(sizeof(next->crc)))
         {
            delete next;
            log(ERROR, "Abrupt end of memory buffer: can't read PNG chunk crc\n");
            return WHEEL_UNEXPECTED_END_OF_FILE;
         }

         next->crc = buffer.read_le<uint32_t>();
         uint32_t crc_check = 0xffffffff;
         crc_check = wcl::update_crc(crc_check, (uint8_t*)next->type, sizeof(uint32_t));
         crc_check = wcl::update_crc(crc_check, (uint8_t*)&(next->data[0]), next->len);
         crc_check ^= 0xffffffff;

         if (next->crc == crc_check)
         {
            chunks.push_back(next);
            if (s == "IEND")
               break;
         } else {
            log(WARNING, "Ignored PNG chunk: ", s, ", failed CRC check\n");
            delete next;
         }
      }

      wcl::string ihdr_tag(chunks[0]->type, 4);
      if (ihdr_tag != "IHDR")
      {
         log(ERROR, "Malformed PNG file\n");
         return WHEEL_INVALID_FORMAT;
      }

      uint8_t bpp, imgtype, cmethod, filter, interlace;

      chunks[0]->data.seek(0);
      *w = chunks[0]->data.read_le<uint32_t>();
      *h = chunks[0]->data.read_le<uint32_t>();

      bpp = chunks[0]->data.read_le<uint8_t>();
      imgtype = chunks[0]->data.read_le<uint8_t>();
      cmethod = chunks[0]->data.read_le<uint8_t>();
      filter = chunks[0]->data.read_le<uint8_t>();
      interlace = chunks[0]->data.read_le<uint8_t>();

      wcl::string type_string;

      if ((bpp < 8) || (cmethod != 0) || (filter != 0))
      {
         log(ERROR, "Unsupported PNG format\n");
         for (int i = 0; i < chunks.size(); ++i) delete chunks[i];
         return WHEEL_INVALID_FORMAT;
      }

      if (interlace == 1)
      {
         log(ERROR, "Adam7 interlaced PNG images not supported\n");
         for (int i = 0; i < chunks.size(); ++i) delete chunks[i];
         return WHEEL_INVALID_FORMAT;
      }

      if (imgtype == 0)
      {
         *c = 1;
         type_string = "greyscale";
      } else if (imgtype == 2) {
         *c = 3;
         type_string = "RGB";
      } else if (imgtype == 3) {
         *c = 1;
         type_string = "indexed";
      } else if (imgtype == 4) {
         *c = 2;
         type_string = "greyscale-alpha";
      } else if (imgtype == 6) {
         *c = 4;
         type_string = "RGBA";
      } else {
         *c = 0;
         type_string = "unrecognized";
      }

      log(FULL_DEBUG, "Reading ",type_string," PNG image: ",*w,"x",*h,", ",
         (uint32_t)bpp," bits per sample. ", *c," channels\n");

      if (imgtype == 3)
      {
         uint32_t plte = ~0;
         uint32_t trns = ~0;

         for (uint32_t it = 0; it < chunks.size(); ++it)
         {
            wcl::string s(chunks[it]->type, 4);
            if (s == "PLTE")
               plte = it;
            else if (s == "tRNS")
               trns = it;
         }

         if (plte == ~0)
         {
            for (int i = 0; i < chunks.size(); ++i) delete chunks[i];
            log(ERROR, "Indexed image without palette\n");

            return WHEEL_INVALID_FORMAT;
         }

         if (chunks[plte]->len % 3 != 0)
         {
            for (int i = 0; i < chunks.size(); ++i) delete chunks[i];
            log(ERROR, "Malformed PLTE chunk in indexed image\n");

            return WHEEL_INVALID_FORMAT;
         }

         uint32_t plte_entries = chunks[plte]->len / 3;
         uint32_t trns_entries = 0;

         if (trns != ~0)
         {
            chunks[trns]->data.seek(0);
            trns_entries = chunks[plte]->len;
         }
         uint8_t r,g,b,a;

         if (palette != nullptr)
            palette->clear();

         for (uint32_t i = 0; i < plte_entries; ++i)
         {
            chunks[plte]->data.seek(3 * i);
            r = chunks[plte]->data.read<uint8_t>();
            g = chunks[plte]->data.read<uint8_t>();
            b = chunks[plte]->data.read<uint8_t>();

            if (i < trns_entries)
               a = chunks[trns]->data.read<uint8_t>();
            else
               a = 0xff;

            // TODO: Do something with the palette entries?

            if (palette != nullptr)
               palette->push_back((r << 24) + (g << 16) + (b << 8) + a);
         }

         log(FULL_DEBUG, "Read ", plte_entries, " palette entries\n");
      }

      wcl::buffer_t concat_data, image_data;
      uint32_t cnum = 0;

      for (auto c : chunks)
      {
         wcl::string s(c->type, 4);
         if (s == "IDAT")
         {
            cnum++;
            log(FULL_DEBUG, "Reading IDAT chunk ",cnum,", size: ",c->len, "B\n");

            concat_data.insert(std::end(concat_data), std::begin(c->data), std::end(c->data));
         }
      }

      z_uncompress((const void*)&concat_data[0], concat_data.size(), image_data);
      log(FULL_DEBUG, "Uncompressed image size: ",image_data.size(), " bytes\n");

      uint8_t scanline_filter, pd;
      size_t col, decoded_bytes = 0;
      int32_t left, top, d;

      if (target == nullptr)
      {
         log(FULL_DEBUG, "targeting nullptr buffer, bailing out\n");
         return WHEEL_ERROR;
      }

      target->clear();

      for (size_t row = 0; row < *h; ++row)
      {
         scanline_filter = image_data.read<uint8_t>();
         col = 0;

         while (col < *w)
         {
            if (bpp == 8)
            {
               for (size_t ch = 0; ch < *c; ++ch)
               {
                  pd = image_data.read<uint8_t>();

                  if (col == 0)
                     left = 0;
                  else
                     left = target->at(target->size() - 1 * *c);

                  if (row == 0)
                     top = 0;
                  else
                     top = target->at(target->size() - *c * *w);

                  if (left == 0 && top == 0)
                     d = 0;
                  else
                  {
                     if (row == 0)
                        d = 0;
                     else if (col == 0)
                        d = left;
                     else
                        d = target->at(target->size() - *c * *w - *c);
                  }

                  if (scanline_filter == 0)
                  {
                     target->push_back(pd);
                  } else if (scanline_filter == 1) {
                     target->push_back(pd + left);
                  } else if (scanline_filter == 2) {
                     target->push_back(pd + top);
                  } else if (scanline_filter == 3) {
                     target->push_back(pd + ((top + left) >> 1));
                  } else if (scanline_filter == 4) {
                     int32_t p = left + top - d;
                     int32_t pa = abs(p - left);
                     int32_t pb = abs(p - top);
                     int32_t pc = abs(p - d);
                     uint8_t pr;

                     if ((pa <= pb) && (pa <= pc))
                        pr = left;
                     else if (pb <= pc)
                        pr = top;
                     else
                        pr = d;

                     target->push_back(pd + pr);
                  }
               }

               decoded_bytes++;
               col++;
            } else {
               log(FATAL, "Unimplemented function: can't decode bpp ",(uint32_t)bpp,"\n");
               assert(0 && "nope");
            }
         }
      }

      log(FULL_DEBUG, "Read ", target->size(), " bytes of image data into buffer\n");

      return WHEEL_OK;
   }
}

namespace yam
{
   namespace format
   {
      constexpr int PNG = 1;
   }

   template<>
   uint32_t load_to_buffer<format::PNG>(const wcl::string& file, image_t& target)
   {
      wcl::buffer_t* png = wcl::GetBuffer(file);
      read_png(*png, &target.width, &target.height, &target.channels, &target.image);
      wcl::DeleteBuffer(file);

      return WHEEL_UNIMPLEMENTED_FEATURE;
   }

   template<>
   uint32_t load_to_texture<format::PNG>(const wcl::string& texture, const wcl::string& file)
   {
      image_t image;
      load_to_buffer<format::PNG>(file, image);

      flip_vertical(image);

//      renderer.CreateTexture(file, image.width, image.height, image.channels);
      renderer.CreateTexture(texture, image);

      return WHEEL_UNIMPLEMENTED_FEATURE;
   }
}
#endif
