#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

enum class token_type
{
	Exit,
	Int,
	Semi,
};

struct token
{
	token_type type;
	std::string lexeme;
};

enum class compile_result
{
	success,
	error,
};

void debug_print(const std::vector<token>& tokens)
{
	for (const auto& token : tokens)
	{
		printf("[%zu] '%s'\n", (size_t)token.type, token.lexeme.data());
	}
}

void tokenize(const std::string& contents, std::vector<token>& tokens)
{
	size_t cursor = 0;
	size_t start = 0;
	char c;

	while (cursor < contents.size())
	{
		start = cursor;
		c = contents[cursor];

		if (isdigit(c))
		{
			while (isdigit(c))
			{
				cursor++;
				c = contents[cursor];
			}

			token tok;
			tok.lexeme = contents.substr(start, cursor - start);
			tok.type = token_type::Int;
			tokens.push_back(tok);
			continue;
		}

		if (isascii(c) && isalpha(c))
		{
			while (isascii(c) && isalnum(c))
			{
				cursor++;
				c = contents[cursor];
			}

			token tok;
			tok.lexeme = contents.substr(start, cursor - start);
			if (tok.lexeme == "exit")
			{
				tok.type = token_type::Exit;
			}
			tokens.push_back(tok);
			continue;
		}

		auto make_token = [&](token_type type)
		{
			cursor++;
			token tok;
			tok.lexeme = contents.substr(start, cursor - start);
			tok.type = type;
			tokens.push_back(tok);
		};

		switch (c)
		{
			case ';': make_token(token_type::Semi); continue;
		}

		cursor++;
	}
}

compile_result compile(const std::vector<token>& tokens, std::string& output)
{
	output += "global _start\n";
	output += "_start:\n";

	for (size_t i = 0; i < tokens.size(); i++)
	{
		token tok = tokens[i];
		if (tok.type == token_type::Exit)
		{
			output += "\tmov rax, 60\n";
			output += "\tmov rdi, " + tokens[++i].lexeme + '\n';
			output += "\tsyscall\n";
		}
	}

	return compile_result::success;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: pluto <input>\n");
        return 1;
    }

    const char* input_path = argv[1];
    if (!std::filesystem::exists(input_path))
    {
        fprintf(stderr, "cannot find %s: No such file or directory\n", input_path);
        return 1;
    }

    std::ifstream file(input_path);
    if (!file.is_open())
    {
        fprintf(stderr, "failed to open file: %s\n", input_path);
        file.close();
        return 1;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    std::string contents(file_size, ' ');
    file.seekg(0, std::ios::beg);
    file.read(contents.data(), file_size);
    file.close();

	std::vector<token> tokens;
	tokenize(contents, tokens);

	debug_print(tokens);

	std::string output;
	compile(tokens, output);

	std::string output_path = "";
    std::filesystem::path input(input_path);
    if (input.has_parent_path())
    {
        std::filesystem::path parent = input.parent_path();
		output_path += parent.string() + '/';
    }

	input.replace_extension(".asm");
	std::string filename = input.filename().string();
	output_path += filename;

    std::ofstream fout(output_path);
	fout.write(output.data(), output.size());
	fout.close();

	std::string nasm_cmd = "nasm ";
	nasm_cmd += "-felf64 ";
	nasm_cmd += output_path;
	printf("%s\n", nasm_cmd.data());
	system(nasm_cmd.data());

	std::filesystem::path out(output_path);
	out.replace_extension(".o");
	std::string link_cmd = "ld ";
	link_cmd += out.string();
	link_cmd += " -o ";
	input.replace_extension("");
	link_cmd += input.string();
	printf("%s\n", link_cmd.data());
	system(link_cmd.data());
}