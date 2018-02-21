#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stdlib.h>

typedef struct HashValue
{
   void*                   value;
   unsigned short          value_size;
}HashValue;

typedef struct HashTableEntry
{
   char*                   key;
   unsigned short          key_size;
   HashValue               v;
   struct HashTableEntry*  next;
}HashTableEntry;

typedef struct HashTable
{
   unsigned int            capacity;
   unsigned int            size;
   struct HashTableEntry** table;
}HashTable;

HashTable*  createHashTable(int size);
void        destroyHashTable(HashTable* hashtable);
int         insertInHashTable(HashTable* hashtable, char* key, unsigned short keysize, void* value, unsigned short valuesize);
void        removeFromHashTable(HashTable* hashtable, char* key, unsigned short keysize);
HashValue*  findInHashTable(HashTable* hashtable, char* key, unsigned short keysize);

#endif //   _HASHTABLE_H_




#ifdef HASHTABLE_IMPLEMENTATION

// http://www.partow.net/programming/hashfunctions/index.html#RSHashFunction
// A simple hash function from Robert Sedgwicks Algorithms in C book. I've added some simple optimizations to the algorithm in order to speed up its hashing process.
unsigned int RSHash(const char* key, unsigned int keysize)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;
   unsigned int i    = 0;

   for (i = 0; i < keysize; ++key, ++i)
   {
      hash = hash * a + (*key);
      a    = a * b;
   }
   return hash;
}

#define HASH      RSHash

#ifdef _DEBUG
static unsigned long alloc_count     = 0;
static unsigned long realloc_count   = 0;
static unsigned long free_count      = 0;

static void* dbg_hash_malloc(size_t size)
{
   ++ alloc_count;
   return malloc(size);
}

static void* dbg_hash_realloc(void* ptr, size_t size)
{
   ++ realloc_count;
   return realloc(ptr, size);
}

static void dbg_hash_free(void* ptr)
{
   ++ free_count;
   free(ptr);
}

#  define MALLOC    dbg_hash_malloc
#  define REALLOC   dbg_hash_realloc
#  define FREE      dbg_hash_free
#else
#  define HASH      RSHash
#  define MALLOC    malloc
#  define REALLOC   realloc
#  define FREE      free
#endif

HashTable* createHashTable(int size)
{
   HashTable* newHashTable = (HashTable*)MALLOC(sizeof(HashTable));
   if(newHashTable == NULL)
      return NULL;

   newHashTable->table = (HashTableEntry**)MALLOC(size * sizeof(HashTableEntry));

   if(newHashTable->table == NULL)
   {
      FREE(newHashTable);
      return NULL;
   }

   for(int i = 0; i < size; ++ i)
   {
      newHashTable->table[i] = NULL;
   }

   newHashTable->capacity = size;
   newHashTable->size = 0;
   return newHashTable;
}

void destroyHashTable(HashTable* hashtable)
{
   if(hashtable == NULL)
      return;

   if(hashtable->table == NULL)
      return;

   // Go through all the possible entries and cleanup
   for(unsigned int i = 0; i < hashtable->capacity; ++ i)
   {
      if(hashtable->table[i] != NULL)
      {
         HashTableEntry* entry = hashtable->table[i];
         do
         {
            HashTableEntry* next_entry = entry->next; // Store next pointer

            FREE(entry->key);
            FREE(entry->v.value);
            FREE(entry);

            entry = next_entry;
         } while(entry != NULL);
      }
   }
   FREE(hashtable->table);
   FREE(hashtable);
}

int insertInHashTable(HashTable* hashtable, char* key, unsigned short keysize, void* value, unsigned short valuesize)
{
   if(hashtable == NULL)
      return -1;

   if(hashtable->table == NULL)
      return -1;

   unsigned int hash = HASH(key, keysize) % hashtable->capacity;

   if(hashtable->table[hash] == NULL)
   {  // Slot was not filled yet
      hashtable->table[hash]                 = (HashTableEntry*)MALLOC(sizeof(HashTableEntry));
      hashtable->table[hash]->next           = NULL;
      hashtable->table[hash]->key_size       = keysize;
      hashtable->table[hash]->v.value_size   = valuesize;

      hashtable->table[hash]->key            = (char*)MALLOC(keysize);
      hashtable->table[hash]->v.value        = (void*)MALLOC(valuesize);
      memcpy(hashtable->table[hash]->key, key, keysize);
      memcpy(hashtable->table[hash]->v.value, value, valuesize);
   }
   else
   {  // The slot is already filled. Traverse down the list add it to the tail
      HashTableEntry* entry = hashtable->table[hash];
      HashTableEntry* prev_entry = 0;

      while(entry != NULL)
      {  // Check if the key is the same.
         if(entry->key != NULL && memcmp(entry->key, key, keysize) == 0)
         {  // The key is equal. We can overwrite the value!
            if(valuesize > entry->v.value_size)
            {  // Need more memory to store the value.
               void* newvalue = (void*)REALLOC(entry->v.value, valuesize);
               if(newvalue)
               {
                  entry->v.value      = newvalue;
                  entry->v.value_size = valuesize;
               }
            }
            return 0;
         }
         prev_entry = entry;
         entry = entry->next;
      }

      if(entry == NULL && prev_entry != NULL)
      {  // New entry at the end of the list.
         prev_entry->next     = (HashTableEntry*)MALLOC(sizeof(HashTableEntry));
         entry                = prev_entry->next;
         entry->next          = NULL;
         entry->key_size      = keysize;
         entry->v.value_size  = valuesize;

         entry->key           = (char*)MALLOC(keysize);
         memcpy(entry->key, key, keysize);

         entry->v.value       = (void*)MALLOC(valuesize);
         memcpy(entry->v.value, value, valuesize);
      }
   }
   ++ hashtable->size;
   return 0;
}

void removeFromHashTable(HashTable* hashtable, char* key, unsigned short keysize)
{
   if(hashtable == NULL)
      return;

   if(hashtable->table == NULL)
      return;

   unsigned int hash = HASH(key, keysize) % hashtable->capacity;
   HashTableEntry* entry = hashtable->table[hash];
   HashTableEntry* prev_entry = NULL;
   while(entry != NULL)
   {
      if(entry->key != NULL && entry->key_size == keysize && memcmp(entry->key, key, keysize) == 0)
      {  // Found the key
         
         if(prev_entry == NULL)
         {  // Remove entry from the table and attach the next in the chain to the head
            hashtable->table[hash] = entry->next;
         }
         else
         {  // Remove entry from the list and attach previous and next link
            prev_entry = entry->next;
         }

         FREE(entry->key);
         FREE(entry->v.value);
         FREE(entry);
         return;
      }
      prev_entry = entry;
      entry = entry->next;
   }
}

HashValue* findInHashTable(HashTable* hashtable, char* key, unsigned short keysize)
{
   if(hashtable == NULL)
      return NULL;

   if(hashtable->table == NULL)
      return NULL;

   unsigned int      hash  = HASH(key, keysize) % hashtable->capacity;
   HashTableEntry*   entry = hashtable->table[hash];
   while(entry != NULL)
   {
      if(entry->key != NULL && entry->key_size == keysize && memcmp(entry->key, key, keysize) == 0)
      {
         return &entry->v;
      }
      entry = entry->next;
   }
   return NULL;
}

#endif   // HASHTABLE_IMPLEMENTATION

