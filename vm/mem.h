#ifndef YESOD_MEM_
# define YESOD_MEM_

# define STACK (0x00000000)

struct yesod_mem {
  uint32_t	m_size;
  uint32_t	s_size;
  void		*memory;
};

#endif /* YESOD_MEM_ */
