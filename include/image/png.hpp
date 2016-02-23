#ifndef YAM_PNG_HPP
#define YAM_PNG_HPP

#include <wheel.h>

#include "../image.h"
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
}

namespace yam
{
   namespace format
   {
      constexpr int PNG = 1;
   }

   template<>
   uint32_t load_to_buffer<format::PNG>(wcl::string& file, image_t& target)
   {
      return WHEEL_UNIMPLEMENTED_FEATURE;
   }

   template<>
   uint32_t load_to_texture<format::PNG>(wcl::string& file, texture_t& target)
   {
      return WHEEL_UNIMPLEMENTED_FEATURE;
   }


   /*
      PNG functions
   */
   uint32_t read_png(const wcl::buffer_t& data, uint32_t* w = nullptr, uint32_t* h = nullptr,
                     uint32_t* c = nullptr, wcl::buffer_t* target = nullptr)
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

      uint32_t idat_count = 0;

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
/*
      *w = *(uint32_t*)(&(chunks[0]->data[0]));
      *h = *(uint32_t*)(&(chunks[0]->data[0]) + 4);
      *h = *(uint32_t*)(&(chunks[0]->data[0]) + 4);
*/
      uint8_t bpp, imgtype, cmethod, filter, interlace;

      chunks[0]->data.seek(0);
      *w = chunks[0]->data.read_le<uint32_t>();
      *h = chunks[0]->data.read_le<uint32_t>();

      bpp = chunks[0]->data.read_le<uint8_t>();
      imgtype = chunks[0]->data.read_le<uint8_t>();
      cmethod = chunks[0]->data.read_le<uint8_t>();
      filter = chunks[0]->data.read_le<uint8_t>();
      interlace = chunks[0]->data.read_le<uint8_t>();

      bool supported_format = true;
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

      log(FULL_DEBUG, "reading ",type_string," PNG image: ",*w,"x",*h,", ", bpp," bits per sample. ",
          *c," channels\n");

      return 0;
   }
}
#endif
