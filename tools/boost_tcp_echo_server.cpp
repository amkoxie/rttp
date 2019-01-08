
#include <cstdlib>
#include <iostream>
#include <string>
#include <deque>
#include <assert.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;

class session : public boost::enable_shared_from_this<session>
{
public:
	session(boost::asio::io_service& io_service)
	: socket_(io_service)
	{
		
	}

	~session()
	{
		std::cout << "connected closed" << std::endl;
	}

	tcp::socket& socket()
	{
	return socket_;
	}

	void start()
	{
		boost::asio::async_read(socket_, boost::asio::buffer(data_, 4),
			boost::bind(&session::handle_read_head, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	void handle_read_head(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (error || bytes_transferred != 4) {
			return;
		}
		else {
			uint32_t len = *(uint32_t*)data_;
			if (len > max_length || len < 4) {
				std::cout << "invalid packet bytes " << len << std::endl;
				return;
			}

			read_bytes_ = len;

			boost::asio::async_read(socket_, boost::asio::buffer(data_+4, len-4),
				boost::bind(&session::handle_read_body, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}

	void handle_read_body(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (!error)
		{
			if (bytes_transferred + 4 != read_bytes_) {
				std::cout << "need read bytes not equals to read bytes" << std::endl;
			}

			std::cout << "read " << bytes_transferred+4 << " bytes" << std::endl;

			send_buffer_deq_.push_back(std::string(data_, bytes_transferred+4));

			if (!sending_) {
				sending_ = true;
				std::cout << "send " << send_buffer_deq_[0].size() << "bytes" << std::endl;
				boost::asio::async_write(socket_,
					boost::asio::buffer(send_buffer_deq_[0].c_str(), send_buffer_deq_[0].size()),
					boost::bind(&session::handle_write, shared_from_this(),
						boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}


			boost::asio::async_read(socket_, boost::asio::buffer(data_, 4),
				boost::bind(&session::handle_read_head, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			std::cout << error.message() << std::endl;
		}
	}

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (!error)
		{
			assert(sending_);
			std::cout << "send complete " << bytes_transferred<< " bytes" << std::endl;

			assert(send_buffer_deq_.size() > 0);
			assert(bytes_transferred <= send_buffer_deq_[0].size());
			send_buffer_deq_[0].erase(0, bytes_transferred);
		
			if (send_buffer_deq_[0].size() == 0)
				send_buffer_deq_.pop_front();
		
			if (send_buffer_deq_.size() > 0) {
				std::cout << "send " << send_buffer_deq_[0].size() << "bytes" << std::endl;
				boost::asio::async_write(socket_,
					boost::asio::buffer(send_buffer_deq_[0].c_str(), send_buffer_deq_[0].size()),
					boost::bind(&session::handle_write, shared_from_this(),
						boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
			else {
				sending_ = false;
			}
		}
		else
		{
			std::cout << error.message() << std::endl;
		}
	}

private:
	tcp::socket socket_;
	enum { max_length = 4096 };
	char data_[max_length];
	int read_bytes_ = 0;
	bool sending_ = false;

	std::deque<std::string> send_buffer_deq_; 
};

class rttp_file_server
{
public:
	rttp_file_server(boost::asio::io_service& io_service, short port)
	: io_service_(io_service),
		acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	{
		boost::shared_ptr<session> new_session;
		new_session.reset(new session(io_service_));

		acceptor_.async_accept(new_session->socket(),
			boost::bind(&rttp_file_server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	void handle_accept(boost::shared_ptr<session> ss,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			ss->start();
			boost::shared_ptr<session> new_session;
			new_session.reset(new session(io_service_));

			acceptor_.async_accept(new_session->socket(),
				boost::bind(&rttp_file_server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
		}
		else
		{
		}
	}

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		rttp_file_server s(io_service, atoi(argv[1]));

		io_service.run();
	}
	catch (std::exception& e)
	{
	std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}